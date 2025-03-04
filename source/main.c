#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <dirent.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <string.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef RUNSHSTR
#define RUNSHSTR "mkdir build\nmkdir install1\ncd build\n../configure --prefix=\"$(realpath ../install1)\"\nmake\nmake install"
#endif
#ifndef SOURCESLISTSTR
#define SOURCESLISTSTR "git https://:@github.com/CoderRC"
#endif

static inline void
mkdim (char *dir, char *envp[])
{
  const char *cdStr = "cd \"";
  const size_t cdStrSize = strlen (cdStr);
  const char *commandmkdir = "\" && test -n \"$(mkdir -p \"";
  const size_t commandmkdirSize = strlen (commandmkdir);
  const char *commandmkdirAfter = "\" 2>&1)\"";
  const size_t commandmkdirAfterSize = strlen (commandmkdirAfter);
  DIR *dirp;
  size_t dirSize;
  size_t nExtra;
  size_t commandSize;
  char *command;
  char *shellLocStr;
  size_t shellLocStrSize;
  shellLocStr = getenv ("SHELL");
  shellLocStrSize = strlen (shellLocStr);
  pid_t spid;
  char *sargv[4];
  siginfo_t ssiginfo;
  if (!dir)
    {
      return;
    }
  *sargv = shellLocStr;
  sargv[1] = "-c";
  sargv[3] = 0;
  for (dirSize = nExtra = 0; dir[dirSize]; dirSize++)
    {
      nExtra += dir[dirSize] == '\'' || dir[dirSize] == '\"'
	|| dir[dirSize] == '$' || dir[dirSize] == '`' || dir[dirSize] == '\\'
	|| dir[dirSize] == '|' || dir[dirSize] == '&' || dir[dirSize] == ';'
	|| dir[dirSize] == '<' || dir[dirSize] == '>';
    }
  dirp = opendir (dir);
  if (dirp)
    {
      closedir (dirp);
      return;
    }
  commandSize =
    cdStrSize + dirSize + commandmkdirSize + commandmkdirAfterSize;
  command = malloc (commandSize);
  memcpy (command, cdStr, cdStrSize);
  commandSize--;
  command[commandSize] = 0;
  commandSize -= commandmkdirAfterSize;
  memcpy (command + commandSize, commandmkdirAfter, commandmkdirAfterSize);
  for (dirSize -= dirSize != 0; dirSize && dir[dirSize] != '/'; dirSize--)
    {
      commandSize--;
      command[commandSize] = dir[dirSize];
      if (dir[dirSize] == '\'' || dir[dirSize] == '\"' || dir[dirSize] == '$'
	  || dir[dirSize] == '`' || dir[dirSize] == '\\'
	  || dir[dirSize] == '|' || dir[dirSize] == '&' || dir[dirSize] == ';'
	  || dir[dirSize] == '<' || dir[dirSize] == '>')
	{
	  commandSize--;
	  command[commandSize] = '\\';
	}
    }
  commandSize--;
  command[commandSize] = 0;
  memcpy (command + commandSize - dirSize, dir, dirSize);
  dirp = opendir (command + commandSize - dirSize);
  for (; !dirp; dirp = opendir (command + commandSize - dirSize))
    {
      command[commandSize] = '/';
      for (dirSize -= dirSize != 0; dirSize && dir[dirSize] != '/'; dirSize--)
	{
	  commandSize--;
	  command[commandSize] = dir[dirSize];
	  if (dir[dirSize] == '\'' || dir[dirSize] == '\"'
	      || dir[dirSize] == '$' || dir[dirSize] == '`'
	      || dir[dirSize] == '\\' || dir[dirSize] == '|'
	      || dir[dirSize] == '&' || dir[dirSize] == ';'
	      || dir[dirSize] == '<' || dir[dirSize] == '>')
	    {
	      commandSize--;
	      command[commandSize] = '\\';
	    }
	}
      commandSize--;
      command[commandSize] = 0;
    }
  closedir (dirp);
  commandSize -= commandmkdirSize - 1;
  memcpy (command + commandSize, commandmkdir, commandmkdirSize);
  for (; dirSize; dirSize--)
    {
      commandSize--;
      command[commandSize] = dir[dirSize - 1];
      if (dir[dirSize - 1] == '\'' || dir[dirSize - 1] == '\"'
	  || dir[dirSize - 1] == '$' || dir[dirSize - 1] == '`'
	  || dir[dirSize - 1] == '\\' || dir[dirSize - 1] == '|'
	  || dir[dirSize - 1] == '&' || dir[dirSize - 1] == ';'
	  || dir[dirSize - 1] == '<' || dir[dirSize - 1] == '>')
	{
	  commandSize--;
	  command[commandSize] = '\\';
	}
    }
  sargv[2] = command;
  if (sargv[2] && !posix_spawnp (&spid, *sargv, NULL, NULL, sargv, envp))
    {
      if (waitid (P_PID, spid, &ssiginfo, WEXITED) != -1)
	{
	  if (ssiginfo.si_signo == SIGCHLD && ssiginfo.si_code == CLD_EXITED)
	    {
	      free (command);
	      return;
	    }
	}
    }
  if (!command)
    {
      commandSize = commandmkdirSize + dirSize + commandmkdirAfterSize + 1;
      command = malloc (commandSize);
    }
  *sargv = "mkdir";
  sargv[1] = 0;
  sargv[2] = 0;
  dirSize = strlen (dir);
  memcpy (command, dir, dirSize);
  commandSize = dirSize;
  command[commandSize] = 0;
  for (dirp = opendir (command); !dirp; dirp = opendir (command))
    {
      for (; commandSize && command[commandSize] != '/'; commandSize--);
      command[commandSize] = 0;
    }
  closedir (dirp);
  command[commandSize] = '/';
  for (;
       commandSize != dirSize && command[commandSize] == '/'; commandSize++);
  for (; commandSize != dirSize && command[commandSize]; commandSize++);
  while (commandSize != dirSize)
    {
      command[commandSize] = 0;
      sargv[1] = command;
      if (sargv[1] && !posix_spawnp (&spid, *sargv, NULL, NULL, sargv, envp))
	{
	  waitid (P_PID, spid, &ssiginfo, WEXITED);
	}
      dirp = opendir (command);
      if (dirp)
	{
	  closedir (dirp);
	}
      else if (commandmkdirSize + 4 <= commandSize
	       && command[commandmkdirSize] == '/'
	       && command[commandmkdirSize + 1] == '/'
	       && command[commandmkdirSize + 2] == '?'
	       && command[commandmkdirSize + 3] == '/')
	{
	  sargv[1] = command + 4;
	  if (command
	      && !posix_spawnp (&spid, *sargv, NULL, NULL, sargv, envp))
	    {
	      waitid (P_PID, spid, &ssiginfo, WEXITED);
	    }
	}
      command[commandSize] = '/';
      for (;
	   commandSize != dirSize
	   && command[commandSize] == '/'; commandSize++);
      for (;
	   commandSize != dirSize && command[commandSize]
	   && command[commandSize] != '/'; commandSize++);
    }
  command[commandSize] = 0;
  sargv[1] = command;
  if (sargv[1] && !posix_spawnp (&spid, *sargv, NULL, NULL, sargv, envp))
    {
      waitid (P_PID, spid, &ssiginfo, WEXITED);
    }
  dirp = opendir (command);
  if (dirp)
    {
      closedir (dirp);
    }
  else if (commandmkdirSize + 4 <= commandSize
	   && command[commandmkdirSize] == '/'
	   && command[commandmkdirSize + 1] == '/'
	   && command[commandmkdirSize + 2] == '?'
	   && command[commandmkdirSize + 3] == '/')
    {
      sargv[1] = command + 4;
      if (command && !posix_spawnp (&spid, *sargv, NULL, NULL, sargv, envp))
	{
	  waitid (P_PID, spid, &ssiginfo, WEXITED);
	}
    }
  free (command);
}

static inline bool
isnewlinechar (char c)
{
  return c == '\n';
}

struct internalListFileEntry
{
  char *fileLoc;
  size_t fileLocSize;
  char *directoryLoc;
  size_t directoryLocSize;
  struct internalListFileEntry *prevdirectory;
  char isdirectory;
  char isdone;
  struct internalListFileEntry *next;
};

struct internalDirectoryScan
{
  DIR *dirp;
  struct internalListFileEntry *iDirectory;
  struct internalListFileEntry *prevdirectory;
  struct internalDirectoryScan *prev;
};

static inline void
appendInternal (char **dstStr, size_t *dstStrSize, char **srcStr,
		size_t *srcStrSize, size_t *dstStrSizeMax)
{
  if (*dstStrSize + *srcStrSize > *dstStrSizeMax)
    {
      char *newStr = (char *) malloc (*dstStrSize + *srcStrSize);
      memcpy (newStr, *dstStr, *dstStrSize);
      free (*dstStr);
      *dstStr = newStr;
      *dstStrSizeMax = *dstStrSize + *srcStrSize;
    }
  memcpy (*dstStr + *dstStrSize, *srcStr, *srcStrSize);
  *dstStrSize += *srcStrSize;
}

static inline void
appendInternalSrcConst (char **dstStr, size_t *dstStrSize,
			const char **srcStr, const size_t *srcStrSize,
			size_t *dstStrSizeMax)
{
  if (*dstStrSize + *srcStrSize > *dstStrSizeMax)
    {
      char *newStr = (char *) malloc (*dstStrSize + *srcStrSize);
      memcpy (newStr, *dstStr, *dstStrSize);
      free (*dstStr);
      *dstStr = newStr;
      *dstStrSizeMax = *dstStrSize + *srcStrSize;
    }
  memcpy (*dstStr + *dstStrSize, *srcStr, *srcStrSize);
  *dstStrSize += *srcStrSize;
}

static inline void
setupDstAppendInternal (char **dstStr, size_t *dstStrSize,
			size_t *dstStrSizeMax)
{
  if (*dstStrSize > *dstStrSizeMax)
    {
      char *newStr = (char *) malloc (*dstStrSize);
      memcpy (newStr, *dstStr, *dstStrSizeMax);
      free (*dstStr);
      *dstStr = newStr;
      *dstStrSizeMax = *dstStrSize;
    }
}

static inline void
loadSourceList (char **packageFileName, size_t *packageFileNameSize,
		char **installDir, size_t *installDirNameSize,
		const char *sourceslistStr, const size_t sourceslistStrSize,
		int *packageFile, size_t *packageFileSize,
		char **packageFileMemMap, struct stat *st,
		size_t *numberOfPackageLists, bool *gotonewline, size_t *i,
		char ***packageList, size_t **packageListNameSizes,
		size_t *packageListNameSize, bool *execute, char *envp[])
{
  *packageFileNameSize = strlen ("etc/a-g/sources.list");
  *packageFileName = malloc (*installDirNameSize + *packageFileNameSize);
  memcpy (*packageFileName, *installDir, *installDirNameSize);
  memcpy (*packageFileName + *installDirNameSize - 1, "etc/a-g/sources.list",
	  *packageFileNameSize);
  *packageFileNameSize += *installDirNameSize - 1;
  (*packageFileName)[*packageFileNameSize] = 0;
  *packageFileNameSize -= strlen ("/sources.list");
  (*packageFileName)[*packageFileNameSize] = 0;
  mkdim (*packageFileName, envp);
  (*packageFileName)[*packageFileNameSize] = '/';
  *packageFileNameSize += strlen ("/sources.list");
  *packageFile =
    open (*packageFileName, O_BINARY | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
  *execute = *packageFile == -1;
  if (*execute)
    {
      *packageFile =
	open (*packageFileName, O_BINARY | O_RDWR | O_CREAT | O_APPEND,
	      S_IRUSR | S_IWUSR);
      *execute = *packageFile != -1;
    }
  else
    {
      fstat (*packageFile, st);
      *execute = st->st_size == 0;
    }
  if (*execute)
    {
      write (*packageFile, sourceslistStr, sourceslistStrSize);
    }
  fstat (*packageFile, st);
  *packageFileSize = st->st_size;
  *packageFileMemMap =
    mmap (0, *packageFileSize, PROT_READ, MAP_SHARED, *packageFile, 0);
  madvise (*packageFileMemMap, *packageFileSize, MADV_WILLNEED);
  *numberOfPackageLists = 0;
  *gotonewline = false;
  for (*i = 0; *i != *packageFileSize; (*i)++)
    {
      if ((*packageFileMemMap)[*i] == '\r')
	{
	  *i += *i + 1 != *packageFileSize;
	}
      if (!*gotonewline)
	{
	  if (*i + 1 != *packageFileSize && *i + 2 != *packageFileSize
	      && *i + 3 != *packageFileSize && *i + 4 != *packageFileSize
	      && !isnewlinechar ((*packageFileMemMap)[*i + 4])
	      && (*packageFileMemMap)[*i + 4] != '\r')
	    {
	      if (!strncmp ((*packageFileMemMap) + *i, "git ", 4))
		{
		  (*numberOfPackageLists)++;
		}
	    }
	  *gotonewline = true;
	}
      if (isnewlinechar ((*packageFileMemMap)[*i]))
	{
	  *gotonewline = false;
	}
    }
  *packageList =
    (char **) malloc (*numberOfPackageLists * sizeof (**packageList));
  *packageListNameSizes =
    (size_t *) malloc (*numberOfPackageLists *
		       sizeof (**packageListNameSizes));
  *numberOfPackageLists = 0;
  *gotonewline = false;
  for (*i = 0; *i != *packageFileSize; (*i)++)
    {
      if ((*packageFileMemMap)[*i] == '\r')
	{
	  *i += (*i + 1) != *packageFileSize;
	}
      if (!*gotonewline)
	{
	  if (*i + 1 != *packageFileSize && *i + 2 != *packageFileSize
	      && *i + 3 != *packageFileSize && *i + 4 != *packageFileSize
	      && !isnewlinechar ((*packageFileMemMap)[*i + 4])
	      && (*packageFileMemMap)[*i + 4] != '\r')
	    {
	      if (!strncmp (*packageFileMemMap + *i, "git ", 4))
		{
		  *i += 4;
		  for (*packageListNameSize = 0;
		       *i + *packageListNameSize != *packageFileSize
		       &&
		       !isnewlinechar ((*packageFileMemMap)
				       [*i + *packageListNameSize])
		       && (*packageFileMemMap)[*i + *packageListNameSize] !=
		       '#'
		       && (*packageFileMemMap)[*i + *packageListNameSize] !=
		       '\r'; (*packageListNameSize)++);
		  (*packageListNameSizes)[*numberOfPackageLists] =
		    *packageListNameSize + 1;
		  (*packageList)[*numberOfPackageLists] =
		    (char *)
		    malloc ((*packageListNameSizes)[*numberOfPackageLists]);
		  memcpy ((*packageList)[*numberOfPackageLists],
			  *packageFileMemMap + *i, *packageListNameSize);
		  (*packageList)[*numberOfPackageLists][*packageListNameSize]
		    = 0;
		  *i += *packageListNameSize;
		  (*numberOfPackageLists)++;
		  if (*i == *packageFileSize)
		    {
		      (*i)--;
		    }
		}
	    }
	  *gotonewline = true;
	}
      if (isnewlinechar ((*packageFileMemMap)[*i]))
	{
	  *gotonewline = false;
	}
    }
}

struct internalListInstalledPackageEntry
{
  char *packageNameStr;
  size_t packageNameStrSize;
  struct internalListInstalledPackageEntry *nextiLIPE;
};

int
main (int argc, char *argv[], char *envp[])
{
  int fieldPrecisionSpecifier;
  size_t *argvSize;
  struct stat st;
  int fd;
  Dl_info dlInfo;
  const char *installDirLocStr = "/installDir.dir";
  const size_t installDirLocStrSize = strlen (installDirLocStr);
  int installDirFile;
  size_t installDirNameSize;
  char *installDir;
  const char *sourceslistStr = "" SOURCESLISTSTR;
  const size_t sourceslistStrSize = strlen (sourceslistStr);
  char *packageFileName;
  size_t packageFileNameSize;
  int packageFile;
  size_t packageFileSize;
  char **packageList;
  size_t *packageListNameSizes;
  bool *packageListUsed;
  size_t numberOfPackageLists;
  size_t packageListNameSize;
  char *packageFileMemMap;
  size_t i;
  size_t j;
  size_t k;
  size_t l;
  size_t m;
  size_t n;
  size_t o;
  size_t p;
  size_t q;
  size_t r;
  size_t s;
  size_t t;
  pid_t spid;
  char *sargv[6];
  siginfo_t ssiginfo;
  bool gotonewline;
  size_t packageNameSize;
  const char *archivesLoc = "var/cache/a-g/archives/";
  const size_t archivesLocSize = strlen (archivesLoc);
  char *startCommandStr;
  size_t startCommandStrPos;
  size_t startCommandStrSize;
  size_t startCommandStrSizeMax;
  char *gitCommandStr = "\" checkout";
  size_t gitCommandStrSize = strlen (gitCommandStr);
  char *shellLocStr;
  size_t shellLocStrSize;
  const char *cdStr = "cd \"";
  const size_t cdStrSize = strlen (cdStr);
  const char *buildLocStr = "/build";
  const size_t buildLocStrSize = strlen (buildLocStr);
  const char *commandSucessStr = "\" && ";
  const size_t commandSucessStrSize = strlen (commandSucessStr);
  const char *shellOptionRunStr = " \"-c\" ";
  const size_t shellOptionRunStrSize = strlen (shellOptionRunStr);
  const char *installLocStr = "/install1";
  const size_t installLocStrSize = strlen (installLocStr);
  const char *infoLocStr = "/var/lib/a-g/info/";
  const size_t infoLocStrSize = strlen (infoLocStr);
  const char *listFileExtensionStr = ".list";
  const size_t listFileExtensionStrSize = strlen (listFileExtensionStr);
  const char *checkIfSymbolicLinkStr = "test -L \"";
  const size_t checkIfSymbolicLinkStrSize = strlen (checkIfSymbolicLinkStr);
  const char *checkIfSymbolicLinkEndStr = "\"";
  const size_t checkIfSymbolicLinkEndStrSize =
    strlen (checkIfSymbolicLinkEndStr);
  int newListFile;
  char *newListFileMemAlloc;
  size_t newListFileMemAllocPos;
  size_t newListFileSize;
  DIR *dirp;
  struct dirent *dir;
  struct internalListFileEntry *startILFE;
  struct internalDirectoryScan *previDS;
  struct internalListFileEntry *currILFE;
  struct internalDirectoryScan *curriDS;
  struct internalListFileEntry *prevdirectory;
  size_t numILFE;
  char *newNameStr;
  size_t newNameStrSize;
  size_t newNameStrSizeMax;
  const char *tmpFileExtensionStr = ".tmp";
  const size_t tmpFileExtensionStrSize = strlen (tmpFileExtensionStr);
  char *tmpNewNameStr;
  size_t tmpNewNameStrSize;
  size_t tmpNewNameStrSizeMax;
  size_t tmpNewNumberPos;
  size_t tmpNewNumberPosMin;
  size_t tmpNewNumberPosMax;
  const char *varLocStr = "/var";
  const size_t varLocStrSize = strlen (varLocStr);
  const char *aGLocStr = "/lib/a-g/";
  const size_t aGLocStrSize = strlen (aGLocStr);
  const char *statusLocStr = "status";
  const size_t statusLocStrSize = strlen (statusLocStr);
  int statusFile;
  char *statusFileMemAlloc;
  size_t statusFileMemAllocPos;
  size_t statusFileSize;
  size_t newstatusFileSize;
  const char *packageEntryStr = "Package: ";
  const size_t packageEntryStrSize = strlen (packageEntryStr);
  const char *statusEntryStr = "Status: ";
  const size_t statusEntryStrSize = strlen (statusEntryStr);
  const char *statusEntryInstalledStr = "install ok installed";
  const size_t statusEntryInstalledStrSize = strlen (statusEntryInstalledStr);
  const char *commitHashEntryStr = "CommitHash: ";
  const size_t commitHashEntryStrSize = strlen (commitHashEntryStr);
  int headFile;
  char *headFileMemMap;
  size_t headFileSize;
  const char *gitLocStr = "/.git/";
  const size_t gitLocStrSize = strlen (gitLocStr);
  const char *HEADLocStr = "HEAD";
  const size_t HEADLocStrSize = strlen (HEADLocStr);
  char *packageListPathStr;
  size_t packageListPathStrSize;
  size_t packageListPathStrSizeMax;
  int *listDelFiles;
  char **listDelFilesMemAlloc;
  size_t listDelFileMemAllocPos;
  size_t *listDelFilesSize;
  struct internalListFileEntry **listDelFilesStartILFE;
  size_t *listDelFilesNumILFE;
  size_t listDelFilesDataSize;
  char *listDelFileTruthAlloc;
  char *currCharPtr;
  const char *gitWorkingDirectoryCommandStr = "git -C \"";
  const size_t gitWorkingDirectoryCommandStrSize =
    strlen (gitWorkingDirectoryCommandStr);
  struct internalListInstalledPackageEntry *firstiLIPE;
  struct internalListInstalledPackageEntry *lastiLIPE;
  bool creatednew;
  char **packageNameStrs;
  size_t *packageNameStrSizes;
  size_t *packageNameStrListi;
  size_t packageNameStrsNum;
  size_t packageNameStrSize;
  const char *configLocStr = "config";
  const size_t configLocStrSize = strlen (configLocStr);
  int configFile;
  char *configFileMemMap;
  size_t configFileMemMapPos;
  size_t configFileSize;
  const char *gitFetchCommandStr = "\" fetch";
  const size_t gitFetchCommandStrSize = strlen (gitFetchCommandStr);
  char *commitHash;
  size_t commitHashSize;
  char *commitHead;
  size_t commitHeadSize;
  char *remoteLoc;
  size_t remoteLocSize;
  char *remoteHash;
  size_t remoteHashSize;
  const char *remotesLocStr = "refs/remotes/";
  const size_t remotesLocStrSize = strlen (remotesLocStr);
  const char *packedRefsLocStr = "packed-refs";
  const size_t packedRefsLocStrSize = strlen (remotesLocStr);
  const char *gitMergeCommandStr = "\" merge";
  const size_t gitMergeCommandStrSize = strlen (gitMergeCommandStr);
  size_t *statusFileRanges[sizeof (size_t) * 8];
  size_t *statusFileAddLoc[sizeof (size_t) * 8];
  size_t *statusFileAddSize[sizeof (size_t) * 8];
  char *statusFileAddStr[sizeof (size_t) * 8];
  size_t *offset[sizeof (size_t) * 8];
  size_t statusFileRangesSize;
  size_t statusFileAddLocSize;
  size_t statusFileAddStrSize;
  size_t statusFileRangesPos;
  size_t statusFileAddLocPos;
  size_t statusFileAddStrPos;
  size_t statusFileRangesSizeMax;
  size_t statusFileAddLocSizeMax;
  size_t statusFileAddStrSizeMax;
  size_t statusFileRangesPosMax;
  size_t statusFileAddLocPosMax;
  size_t remaining;
  size_t *statusFileAddStRSize;
  size_t statusFileAddStrLocPos;
  size_t *radixes1;
  size_t *radixoffPos1;
  size_t *radixoffSize1;
  size_t *radix1;
  size_t selectedLength1;
  size_t *radixes2;
  size_t *radixoffPos2;
  size_t *radixoffSize2;
  size_t *radix2;
  size_t selectedLength2;
  size_t s1;
  size_t s2;
  size_t lPos1;
  size_t lSize1;
  size_t lPos2;
  size_t lSize2;
  bool statusFileRequestSorted;
  bool deloccured1;
  const char *directorySeparator = "/";
  const size_t directorySeparatorSize = strlen (directorySeparator);
  const char *configurationLoc = "etc/a-g/";
  const size_t configurationLocSize = strlen (configurationLoc);
  const char *defaulticLoc = "default/";
  const size_t defaulticLocSize = strlen (defaulticLoc);
  const char *overrideicLoc = "override/";
  const size_t overrideicLocSize = strlen (overrideicLoc);
  const char *additionalicLoc = "additional/";
  const size_t additionalicLocSize = strlen (additionalicLoc);
  const char *runshLocStr = "run/run.sh";
  const size_t runshLocStrSize = strlen (runshLocStr);
  const char *runconfigLocStr = "runconfig/";
  const size_t runconfigLocStrSize = strlen (runconfigLocStr);
  bool shellSpace;
  bool execute;
  char quoteType;
  size_t nQuotes;
  const char *runshStr = "" RUNSHSTR;
  const size_t runshStrSize = strlen (runshStr);
  const char *configStrs[] = { "configure.conf", "make.conf", "", "" };
  const char *configStrsLoc[] = { "4", "5", "configure.conf", "make.conf" };
  const size_t configStrsNums[] = { 2, 2 };
  const char *configStrsDir[] = { "runconfig", "config" };
  size_t configStrsLen[sizeof (configStrs) / sizeof (*configStrs)];
  size_t configStrsLocLen[sizeof (configStrsLoc) / sizeof (*configStrsLoc)];
  size_t configStrsDirLen[sizeof (configStrsDir) / sizeof (*configStrsDir)];
  const char *runshDirLocStr = "run/";
  const size_t runshDirLocStrSize = strlen (runshDirLocStr);
  int runFile;
  char *runFileMemAlloc;
  size_t runFileMemAllocPos;
  size_t runFileSize;
  size_t runFileLine;
  int runconfig;
  char *runconfigMemAlloc;
  size_t runconfigMemAllocPos;
  size_t runconfigSize;
  int config;
  char *configMemAlloc;
  size_t configMemAllocPos;
  size_t configSize;
  for (i = 0; i != sizeof (configStrs) / sizeof (*configStrs); i++)
    {
      configStrsLen[i] = strlen (configStrs[i]);
    }
  for (i = 0; i != sizeof (configStrsLoc) / sizeof (*configStrsLoc); i++)
    {
      configStrsLocLen[i] = strlen (configStrsLoc[i]);
    }
  for (i = 0; i != sizeof (configStrsDir) / sizeof (*configStrsDir); i++)
    {
      configStrsDirLen[i] = strlen (configStrsDir[i]);
    }
  statusFileRequestSorted = false;
  installDirNameSize = 0;
  dlInfo.dli_fname = 0;
  dlInfo.dli_fbase = 0;
  dlInfo.dli_sname = 0;
  dlInfo.dli_saddr = 0;
  dladdr (&main, &dlInfo);
  for (j = i = 0; dlInfo.dli_fname[i]; i++)
    {
      k = dlInfo.dli_fname[i] == '/' || dlInfo.dli_fname[i] == '\\';
      k--;
      installDirNameSize = (i & (0 - k - 1)) + (installDirNameSize & k);
    }
  installDir =
    (char *) malloc (installDirNameSize + installDirLocStrSize + 1);
  for (i = 0; i != installDirNameSize; i++)
    {
      installDir[i] = dlInfo.dli_fname[i];
      j = installDir[i] == '\\';
      installDir[i] = (installDir[i] & (j - 1)) + ('/' & (0 - j));
    }
  memcpy (installDir + installDirNameSize, installDirLocStr,
	  installDirLocStrSize);
  installDirNameSize += installDirLocStrSize;
  installDir[installDirNameSize] = 0;
  installDirFile = open (installDir, O_BINARY | O_RDWR);
  if (installDirFile == -1)
    {
      for (; installDirNameSize && installDir[installDirNameSize] != '/';
	   installDirNameSize--);
      installDirNameSize -= installDirNameSize != 0;
      for (; installDirNameSize && installDir[installDirNameSize] != '/';
	   installDirNameSize--);
      installDir[installDirNameSize] = '/';
      installDirNameSize++;
      installDir[installDirNameSize] = 0;
      installDirNameSize++;
    }
  else
    {
      fstat (installDirFile, &st);
      free (installDir);
      installDirNameSize = st.st_size;
      installDir = (char *) malloc (installDirNameSize + 2);
      read (installDirFile, installDir, installDirNameSize);
      close (installDirFile);
      for (i = 0; i != installDirNameSize; i++)
	{
	  if (installDir[i] == '\\')
	    {
	      installDir[i] = '/';
	    }
	}
      installDirFile = -1;
      installDir[installDirNameSize] = '/';
      installDirNameSize++;
      installDir[installDirNameSize] = 0;
      installDirNameSize++;
    }
  if (argc != 0 && argc != 1)
    {
      argvSize = malloc (argc * sizeof (*argvSize));
      for (i = 2; i != argc; i++)
	{
	  argvSize[i] = strlen (argv[i]);
	}
      if (!strcmp (argv[1], "install") && argc != 2)
	{
	  printf ("Reading package lists... ");
	  loadSourceList (&packageFileName, &packageFileNameSize, &installDir,
			  &installDirNameSize, sourceslistStr,
			  sourceslistStrSize, &packageFile, &packageFileSize,
			  &packageFileMemMap, &st, &numberOfPackageLists,
			  &gotonewline, &i, &packageList,
			  &packageListNameSizes, &packageListNameSize,
			  &execute, envp);
	  printf ("Done\n");
	  printf ("Reading state information... ");
	  printf ("Done\n");
	  printf ("The following NEW package will be installed:\n");
	  printf (" ");
	  for (i = 2; i != argc; i++)
	    {
	      printf (" %s", argv[i]);
	    }
	  printf ("\n");
	  printf
	    ("0 upgraded, %d newly installed, 0 to remove and 0 not upgraded.\n",
	     argc - 2);
	  printf ("Do you want to continue? [Y/n] ");
	  if (getchar () != 'n')
	    {
	      startCommandStrSizeMax = 1;
	      startCommandStr = malloc (startCommandStrSizeMax);
	      *sargv = 0;
	      sargv[2] = "--no-checkout";
	      sargv[5] = 0;
	      for (i = 2; i != argc; i++)
		{
		  if (*sargv)
		    {
		      waitid (P_PID, spid, &ssiginfo, WEXITED);
		    }
		  *sargv = 0;
		  sargv[1] = "clone";
		  for (j = 0; j != numberOfPackageLists; j++)
		    {
		      packageNameSize = strlen (argv[i]);
		      startCommandStrSize = packageListNameSizes[j] +
			packageNameSize * 2 + installDirNameSize +
			archivesLocSize + 1;
		      setupDstAppendInternal (&startCommandStr,
					      &startCommandStrSize,
					      &startCommandStrSizeMax);
		      memcpy (startCommandStr, packageList[j],
			      packageListNameSizes[j]);
		      startCommandStr[packageListNameSizes[j] - 1] = '/';
		      memcpy (startCommandStr +
			      packageListNameSizes[j], argv[i],
			      packageNameSize);
		      sargv[3] = startCommandStr;
		      sargv[4] = startCommandStr + packageListNameSizes[j] +
			packageNameSize;
		      sargv[4][0] = 0;
		      sargv[4]++;
		      memcpy (startCommandStr +
			      packageListNameSizes[j] + packageNameSize + 1,
			      installDir, installDirNameSize);
		      memcpy (startCommandStr +
			      packageListNameSizes[j] + packageNameSize +
			      installDirNameSize, archivesLoc,
			      archivesLocSize);
		      memcpy (startCommandStr +
			      packageListNameSizes[j] + packageNameSize +
			      installDirNameSize + archivesLocSize, argv[i],
			      packageNameSize);
		      startCommandStr[startCommandStrSize - 1] = 0;
		      if (*sargv
			  && waitid (P_PID, spid, &ssiginfo, WEXITED) != -1)
			{
			  if (ssiginfo.si_signo == SIGCHLD
			      && ssiginfo.si_code == CLD_EXITED
			      && !ssiginfo.si_status)
			    {
			      sargv[1] = 0;
			      j = numberOfPackageLists - 1;
			    }
			}
		      printf ("Get:1 %s main amd64 %s [0 kB]\n",
			      packageList[j], argv[i]);
		      if (sargv[1])
			{
			  *sargv = "git";
			}
		      if (*sargv
			  && posix_spawnp (&spid, *sargv, NULL, NULL, sargv,
					   envp) != 0)
			{
			  *sargv = 0;
			}
		    }
		}
	      newNameStr = malloc (1);
	      newNameStrSize = 0;
	      newNameStrSizeMax = 1;
	      tmpNewNameStr = malloc (1);
	      tmpNewNameStrSize = 0;
	      tmpNewNameStrSizeMax = 1;
	      *sargv = 0;
	      sargv[3] = "checkout";
	      sargv[4] = 0;
	      if (sargv[1] && numberOfPackageLists)
		{
		  waitid (P_PID, spid, &ssiginfo, WEXITED);
		}
	      sargv[1] = "-C";
	      for (i = 2; i != argc; i++)
		{
		  packageNameSize = strlen (argv[i]);
		  startCommandStrSize =
		    installDirNameSize - 1 + archivesLocSize +
		    packageNameSize + 1;
		  setupDstAppendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &startCommandStrSizeMax);
		  sargv[2] = startCommandStr;
		  memcpy (startCommandStr, installDir, installDirNameSize);
		  memcpy (startCommandStr +
			  installDirNameSize - 1, archivesLoc,
			  archivesLocSize);
		  memcpy (startCommandStr +
			  installDirNameSize - 1 + archivesLocSize, argv[i],
			  packageNameSize);
		  startCommandStr[startCommandStrSize - 1] = 0;
		  if (*sargv)
		    {
		      waitid (P_PID, spid, &ssiginfo, WEXITED);
		    }
		  printf ("Selecting previously unselected package %s.\n",
			  argv[i]);
		  printf ("Preparing to unpack \"%s\" ...\n", argv[i]);
		  printf ("Unpacking %s ...\n", argv[i]);
		  if (sargv[2])
		    {
		      *sargv = "git";
		    }
		  if (*sargv
		      && posix_spawnp (&spid, *sargv, NULL, NULL, sargv,
				       envp) != 0)
		    {
		      *sargv = 0;
		    }
		}
	      if (argc - 2)
		{
		  waitid (P_PID, spid, &ssiginfo, WEXITED);
		}
	      statusFileMemAllocPos = 0;
	      selectedLength1 = 0;
	      selectedLength2 = 0;
	      statusFileRangesPos = 0;
	      statusFileAddLocPos = 0;
	      statusFileAddStrPos = 0;
	      statusFileRangesSize = 0;
	      statusFileAddLocSize = 0;
	      statusFileAddStrSize = 0;
	      statusFileRangesPosMax = 0;
	      statusFileAddLocPosMax = 0;
	      statusFileRangesSizeMax = 0;
	      statusFileAddLocSizeMax = 0;
	      statusFileAddStrSizeMax = 0;
	      *statusFileRanges = (size_t *) malloc (sizeof (size_t) * 2);
	      *statusFileAddLoc = (size_t *) malloc (sizeof (size_t));
	      *statusFileAddSize = (size_t *) malloc (sizeof (size_t));
	      *statusFileAddStr = (char *) malloc (sizeof (char));
	      statusFileRanges[0][0] = statusFileRanges[0][1] =
		statusFileAddLoc[0][0] = statusFileAddSize[0][0] = 0;
	      statusFileAddStrLocPos = 0;
	      shellLocStr = getenv ("SHELL");
	      shellLocStrSize = strlen (shellLocStr);
	      newNameStrSizeMax = 1;
	      newNameStr = (char *) malloc (newNameStrSizeMax);
	      newNameStrSize = 0;
	      shellSpace = true;
	      for (j = 0; j != shellLocStrSize; j++)
		{
		  shellSpace &= shellLocStr[j] != ' ';
		}
	      shellSpace = 1 - shellSpace;
	      for (i = 2; i != argc; i++)
		{
		  packageNameStrSize = 1;
		  packageNameStrSize <<= sizeof (fieldPrecisionSpecifier) *
		    8 - 1;
		  packageNameStrSize--;
		  fieldPrecisionSpecifier = packageNameStrSize;
		  packageNameStrSize = argvSize[i];
		  printf ("Setting up %s ...\n", argv[i]);
		  startCommandStrSize =
		    cdStrSize + (installDirNameSize + archivesLocSize +
				 argvSize[i]) * 2 + buildLocStrSize +
		    commandSucessStrSize + shellLocStrSize +
		    shellOptionRunStrSize;
		  newNameStrSize = 0;
		  installDirNameSize--;
		  appendInternal (&newNameStr, &newNameStrSize, &installDir,
				  &installDirNameSize, &newNameStrSizeMax);
		  installDirNameSize++;
		  appendInternalSrcConst (&newNameStr, &newNameStrSize,
					  &configurationLoc,
					  &configurationLocSize,
					  &newNameStrSizeMax);
		  appendInternalSrcConst (&newNameStr, &newNameStrSize,
					  &overrideicLoc, &overrideicLocSize,
					  &newNameStrSizeMax);
		  appendInternal (&newNameStr, &newNameStrSize, &argv[i],
				  &argvSize[i], &newNameStrSizeMax);
		  appendInternalSrcConst (&newNameStr, &newNameStrSize,
					  &directorySeparator,
					  &directorySeparatorSize,
					  &newNameStrSizeMax);
		  appendInternalSrcConst (&newNameStr, &newNameStrSize,
					  &runshLocStr, &runshLocStrSize,
					  &newNameStrSizeMax);
		  newNameStrSize++;
		  setupDstAppendInternal (&newNameStr, &newNameStrSize,
					  &newNameStrSizeMax);
		  newNameStr[newNameStrSize - 1] = 0;
		  setupDstAppendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &startCommandStrSizeMax);
		  startCommandStrSize = 0;
		  *sargv = 0;
		  sargv[1] = "-c";
		  sargv[3] = 0;
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &cdStr,
					  &cdStrSize,
					  &startCommandStrSizeMax);
		  installDirNameSize--;
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  &installDir, &installDirNameSize,
				  &startCommandStrSizeMax);
		  installDirNameSize++;
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &archivesLoc,
					  &archivesLocSize,
					  &startCommandStrSizeMax);
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  &argv[i], &argvSize[i],
				  &startCommandStrSizeMax);
		  startCommandStrPos = startCommandStrSize;
		  runFile =
		    open (newNameStr,
			  O_BINARY | O_RDONLY | O_CREAT | O_APPEND,
			  S_IRUSR | S_IWUSR);
		  runFileMemAllocPos = 0;
		  runFileLine = 0;
		  runconfig = -1;
		  runconfigSize = runconfigMemAllocPos = 0;
		  runconfigMemAlloc = MAP_FAILED;
		  config = -1;
		  configSize = configMemAllocPos = 0;
		  configMemAlloc = MAP_FAILED;
		  gotonewline = false;
		  newNameStrSize -=
		    overrideicLocSize + argvSize[i] + directorySeparatorSize +
		    runshLocStrSize + 1;
		  if (runFile != -1)
		    {
		      fstat (runFile, &st);
		      runFileSize = st.st_size;
		      runFileMemAlloc =
			mmap (0, runFileSize, PROT_READ, MAP_SHARED, runFile,
			      0);
		      madvise (runFileMemAlloc, runFileSize, MADV_WILLNEED);
		      execute = false;
		      for (j = 0; j != runFileSize; j++)
			{
			  if (runFileMemAlloc[j] == '\r')
			    {
			      j += j + 1 != runFileSize;
			    }
			  if (!gotonewline)
			    {
			      runFileLine++;
			      appendInternalSrcConst (&newNameStr,
						      &newNameStrSize,
						      &overrideicLoc,
						      &overrideicLocSize,
						      &newNameStrSizeMax);
			      appendInternal (&newNameStr, &newNameStrSize,
					      &argv[i], &argvSize[i],
					      &newNameStrSizeMax);
			      appendInternalSrcConst (&newNameStr,
						      &newNameStrSize,
						      &directorySeparator,
						      &directorySeparatorSize,
						      &newNameStrSizeMax);
			      appendInternalSrcConst (&newNameStr,
						      &newNameStrSize,
						      &runconfigLocStr,
						      &runconfigLocStrSize,
						      &newNameStrSizeMax);
			      lPos1 = runFileLine;
			      for (lSize1 = 0; lPos1; lSize1++)
				{
				  newNameStrSize++;
				  setupDstAppendInternal (&newNameStr,
							  &newNameStrSize,
							  &newNameStrSizeMax);
				  newNameStr[newNameStrSize - 1] =
				    '0' + lPos1 % 10;
				  lPos1 /= 10;
				}
			      for (; lPos1 != lSize1 >> 1; lPos1++)
				{
				  k =
				    newNameStr[newNameStrSize + lPos1 -
					       lSize1];
				  newNameStr[newNameStrSize + lPos1 -
					     lSize1] =
				    newNameStr[newNameStrSize - 1 - lPos1];
				  newNameStr[newNameStrSize - lPos1 - 1] = k;
				}
			      newNameStrSize++;
			      setupDstAppendInternal (&newNameStr,
						      &newNameStrSize,
						      &newNameStrSizeMax);
			      newNameStr[newNameStrSize - 1] = 0;
			      runconfig =
				open (newNameStr,
				      O_BINARY | O_RDWR | O_APPEND,
				      S_IRUSR | S_IWUSR);
			      runconfigMemAllocPos = 0;
			      fstat (runconfig, &st);
			      runconfigSize = st.st_size;
			      runconfigMemAlloc =
				mmap (0, runconfigSize, PROT_READ, MAP_SHARED,
				      runconfig, 0);
			      nQuotes = 0;
			      runFileMemAllocPos = j;
			      execute = j != runFileSize;
			      while (!nQuotes)
				{
				  for (;
				       j != runFileSize
				       && runFileMemAlloc[j] == ' '; j++);
				  for (;
				       j != runFileSize
				       && runFileMemAlloc[j] == '\"'; j++)
				    {
				      nQuotes++;
				    }
				  execute = j != runFileSize;
				  execute = execute
				    && runFileMemAlloc[j] == 'c';
				  j += execute;
				  if (execute)
				    {
				      for (;
					   j != runFileSize
					   && runFileMemAlloc[j] == '\"'; j++)
					{
					  nQuotes++;
					}
				    }
				  execute = execute && j != runFileSize;
				  execute = execute
				    && runFileMemAlloc[j] == 'd';
				  if (!nQuotes)
				    {
				      if (runFileMemAlloc[j] == '=')
					{
					  break;
					}
				      else
					{
					  for (;
					       j != runFileSize
					       && runFileMemAlloc[j] != '='
					       && runFileMemAlloc[j] != ' '
					       && runFileMemAlloc[j] != '\"'
					       &&
					       !isnewlinechar (runFileMemAlloc
							       [j]); j++);
					  if (j == runFileSize
					      || runFileMemAlloc[j] == ' '
					      || runFileMemAlloc[j] == '\"'
					      ||
					      isnewlinechar (runFileMemAlloc
							     [j]))
					    {
					      break;
					    }
					  if (runFileMemAlloc[j] == '=')
					    {
					      for (;
						   j != runFileSize
						   && runFileMemAlloc[j] !=
						   ' '
						   &&
						   !isnewlinechar
						   (runFileMemAlloc[j]); j++)
						{
						  if (runFileMemAlloc[j] ==
						      '\"')
						    {
						      for (;
							   j != runFileSize
							   &&
							   runFileMemAlloc[j]
							   != '\"'; j++);
						    }
						}
					      nQuotes = j == runFileSize;
					    }
					}
				    }
				}
			      j += execute && j != runFileSize
				&& runFileMemAlloc[j] == 'd';
			      if (execute)
				{
				  for (;
				       j != runFileSize
				       && runFileMemAlloc[j] == '\"'; j++)
				    {
				      nQuotes++;
				    }
				}
			      execute = execute && 1 - (nQuotes & 1);
			      execute = execute && j != runFileSize;
			      if (execute)
				{
				  for (;
				       j != runFileSize
				       && runFileMemAlloc[j] == ' '; j++);
				}
			      execute = 1 - execute;
			      execute = execute && j != runFileSize;
			      if (execute)
				{
				  j = runFileMemAllocPos;
				  appendInternalSrcConst (&startCommandStr,
							  &startCommandStrSize,
							  &commandSucessStr,
							  &commandSucessStrSize,
							  &startCommandStrSizeMax);
				}
			      else
				{
				  while (j != runFileSize
					 &&
					 !isnewlinechar (runFileMemAlloc[j])
					 && !(runFileMemAlloc[j] == '\r'
					      && j + 1 != runFileSize
					      &&
					      isnewlinechar (runFileMemAlloc
							     [j + 1])))
				    {
				      for (;
					   j != runFileSize
					   && (runFileMemAlloc[j] == '/'
					       || runFileMemAlloc[j] == '\"');
					   j++)
					{
					  nQuotes +=
					    runFileMemAlloc[j] == '\"';
					}
				      for (;
					   j != runFileSize
					   && (runFileMemAlloc[j] == '\\'
					       || runFileMemAlloc[j] == '\"');
					   j++)
					{
					  nQuotes +=
					    runFileMemAlloc[j] == '\"';
					}
				      if (1 - (nQuotes & 1)
					  && j != runFileSize
					  && runFileMemAlloc[j] == ' ')
					{
					  break;
					}
				      if (runFileMemAlloc[j] == '.')
					{
					  if (j + 1 != runFileSize
					      && runFileMemAlloc[j + 1] ==
					      '.')
					    {
					      if (j + 2 != runFileSize
						  && (runFileMemAlloc[j + 2]
						      == '/'
						      || runFileMemAlloc[j +
									 2] ==
						      '\\'
						      ||
						      isnewlinechar
						      (runFileMemAlloc[j + 2])
						      ||
						      (runFileMemAlloc[j + 2]
						       == '\r'
						       && j + 3 != runFileSize
						       &&
						       isnewlinechar
						       (runFileMemAlloc
							[j + 3]))))
						{
						  for (;
						       startCommandStrSize !=
						       cdStrSize
						       &&
						       startCommandStr
						       [startCommandStrSize]
						       != '/'
						       &&
						       startCommandStr
						       [startCommandStrSize]
						       != '\\';
						       startCommandStrSize--);
						  execute = true;
						}
					    }
					  else if (j + 1 != runFileSize
						   && (runFileMemAlloc[j + 1]
						       == '/'
						       || runFileMemAlloc[j +
									  1]
						       == '\\'
						       ||
						       isnewlinechar
						       (runFileMemAlloc
							[j + 1])
						       ||
						       (runFileMemAlloc[j + 1]
							== '\r'
							&& j + 2 !=
							runFileSize
							&&
							isnewlinechar
							(runFileMemAlloc
							 [j + 2]))))
					    {
					      execute = true;
					    }
					}
				      if (execute)
					{
					  for (;
					       j != runFileSize
					       && runFileMemAlloc[j] == '.';
					       j++);
					}
				      else
					{
					  appendInternalSrcConst
					    (&startCommandStr,
					     &startCommandStrSize,
					     &directorySeparator,
					     &directorySeparatorSize,
					     &startCommandStrSizeMax);
					  for (;
					       j != runFileSize
					       && !(1 - (nQuotes & 1)
						    && runFileMemAlloc[j] ==
						    ' ')
					       && runFileMemAlloc[j] != '/'
					       && runFileMemAlloc[j] != '\\'
					       &&
					       !isnewlinechar (runFileMemAlloc
							       [j])
					       && !(runFileMemAlloc[j] == '\r'
						    && j + 1 != runFileSize
						    &&
						    isnewlinechar
						    (runFileMemAlloc[j + 1]));
					       j++)
					    {
					      if (runFileMemAlloc[j] == '\"')
						{
						  nQuotes++;
						}
					      else
						{
						  startCommandStrSize++;
						  setupDstAppendInternal
						    (&startCommandStr,
						     &startCommandStrSize,
						     &startCommandStrSizeMax);
						  startCommandStr
						    [startCommandStrSize -
						     1] = runFileMemAlloc[j];
						}
					    }
					}
				      execute = false;
				    }
				  startCommandStrPos = startCommandStrSize;
				}
			      newNameStrSize -=
				runconfigLocStrSize + lSize1 + 1;
			      configMemAlloc = runconfigMemAlloc;
			      if (runconfigMemAlloc != MAP_FAILED)
				{
				  madvise (runconfigMemAlloc, runconfigSize,
					   MADV_WILLNEED);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &configLocStr,
							  &configLocStrSize,
							  &newNameStrSizeMax);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &directorySeparator,
							  &directorySeparatorSize,
							  &newNameStrSizeMax);
				  appendInternal (&newNameStr,
						  &newNameStrSize,
						  &runconfigMemAlloc,
						  &runconfigSize,
						  &newNameStrSizeMax);
				  newNameStrSize++;
				  setupDstAppendInternal (&newNameStr,
							  &newNameStrSize,
							  &newNameStrSizeMax);
				  newNameStr[newNameStrSize - 1] = 0;
				  newNameStrSize -=
				    configLocStrSize +
				    directorySeparatorSize + runconfigSize +
				    1;
				  munmap (runconfigMemAlloc, runconfigSize);
				  config =
				    open (newNameStr,
					  O_BINARY | O_RDWR | O_APPEND,
					  S_IRUSR | S_IWUSR);
				  configMemAllocPos = 0;
				  fstat (config, &st);
				  configSize = st.st_size;
				  configMemAlloc =
				    mmap (0, configSize, PROT_READ,
					  MAP_SHARED, config, 0);
				  if (configMemAlloc != MAP_FAILED)
				    {
				      madvise (configMemAlloc, configSize,
					       MADV_WILLNEED);
				    }
				}
			      newNameStrSize -=
				overrideicLocSize + argvSize[i] +
				directorySeparatorSize;
			      gotonewline = true;
			    }
			  if (isnewlinechar (runFileMemAlloc[j]))
			    {
			      gotonewline = false;
			    }
			  if (execute && gotonewline)
			    {
			      startCommandStrSize++;
			      setupDstAppendInternal (&startCommandStr,
						      &startCommandStrSize,
						      &startCommandStrSizeMax);
			      startCommandStr[startCommandStrSize - 1] =
				runFileMemAlloc[j];
			    }
			  else if (execute)
			    {
			      if (configMemAlloc != MAP_FAILED)
				{
				  for (quoteType = nQuotes =
				       configMemAllocPos = 0;
				       configMemAllocPos != configSize;
				       configMemAllocPos++)
				    {
				      if (!gotonewline)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] = ' ';
					  execute = gotonewline = true;
					}
				      if (isnewlinechar
					  (configMemAlloc[configMemAllocPos]))
					{
					  gotonewline = false;
					}
				      if (configMemAlloc[configMemAllocPos] ==
					  '\r'
					  && configMemAllocPos + 1 !=
					  configSize
					  &&
					  isnewlinechar (configMemAlloc
							 [configMemAllocPos +
							  1]))
					{
					  configMemAllocPos++;
					  gotonewline = false;
					}
				      if (execute
					  && configMemAlloc[configMemAllocPos]
					  == '#' && (!configMemAllocPos
						     ||
						     configMemAlloc
						     [configMemAllocPos -
						      1] == ' '
						     ||
						     isnewlinechar
						     (configMemAlloc
						      [configMemAllocPos -
						       1]))
					  && 1 - (nQuotes & 1))
					{
					  execute = false;
					}
				      if ((execute
					   &&
					   configMemAlloc[configMemAllocPos]
					   == '\"')
					  || configMemAlloc[configMemAllocPos]
					  == '\'')
					{
					  if (1 - (nQuotes & 1))
					    {
					      quoteType =
						configMemAlloc
						[configMemAllocPos];
					    }
					  nQuotes +=
					    quoteType ==
					    configMemAlloc[configMemAllocPos];
					}
				      if (gotonewline & execute)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] =
					    configMemAlloc[configMemAllocPos];
					}
				    }
				}
			      startCommandStrSize++;
			      setupDstAppendInternal (&startCommandStr,
						      &startCommandStrSize,
						      &startCommandStrSizeMax);
			      startCommandStr[startCommandStrSize - 1] = 0;
			      sargv[2] = startCommandStr;
			      if (*sargv)
				{
				  waitid (P_PID, spid, &ssiginfo, WEXITED);
				}
			      if (sargv[2])
				{
				  *sargv = shellLocStr;
				}
			      if (*sargv
				  && posix_spawnp (&spid, *sargv, NULL, NULL,
						   sargv, envp) != 0)
				{
				  *sargv = 0;
				}
			      startCommandStrSize = startCommandStrPos;
			      execute = true;
			      gotonewline = false;
			    }
			}
		      if (execute)
			{
			  if (configMemAlloc != MAP_FAILED)
			    {
			      for (quoteType = nQuotes = configMemAllocPos =
				   0; configMemAllocPos != configSize;
				   configMemAllocPos++)
				{
				  if (!gotonewline)
				    {
				      startCommandStrSize++;
				      setupDstAppendInternal
					(&startCommandStr,
					 &startCommandStrSize,
					 &startCommandStrSizeMax);
				      startCommandStr[startCommandStrSize -
						      1] = ' ';
				      execute = gotonewline = true;
				    }
				  if (isnewlinechar
				      (configMemAlloc[configMemAllocPos]))
				    {
				      gotonewline = false;
				    }
				  if (configMemAlloc[configMemAllocPos] ==
				      '\r'
				      && configMemAllocPos + 1 != configSize
				      &&
				      isnewlinechar (configMemAlloc
						     [configMemAllocPos + 1]))
				    {
				      configMemAllocPos++;
				      gotonewline = false;
				    }
				  if (execute
				      && configMemAlloc[configMemAllocPos] ==
				      '#' && (!configMemAllocPos
					      ||
					      configMemAlloc[configMemAllocPos
							     - 1] == ' '
					      ||
					      isnewlinechar (configMemAlloc
							     [configMemAllocPos
							      - 1]))
				      && 1 - (nQuotes & 1))
				    {
				      execute = false;
				    }
				  if ((execute
				       && configMemAlloc[configMemAllocPos] ==
				       '\"')
				      || configMemAlloc[configMemAllocPos] ==
				      '\'')
				    {
				      if (1 - (nQuotes & 1))
					{
					  quoteType =
					    configMemAlloc[configMemAllocPos];
					}
				      nQuotes +=
					quoteType ==
					configMemAlloc[configMemAllocPos];
				    }
				  if (gotonewline & execute)
				    {
				      startCommandStrSize++;
				      setupDstAppendInternal
					(&startCommandStr,
					 &startCommandStrSize,
					 &startCommandStrSizeMax);
				      startCommandStr[startCommandStrSize -
						      1] =
					configMemAlloc[configMemAllocPos];
				    }
				}
			    }
			  startCommandStrSize++;
			  setupDstAppendInternal (&startCommandStr,
						  &startCommandStrSize,
						  &startCommandStrSizeMax);
			  startCommandStr[startCommandStrSize - 1] = 0;
			  sargv[2] = startCommandStr;
			  if (*sargv)
			    {
			      waitid (P_PID, spid, &ssiginfo, WEXITED);
			    }
			  if (sargv[2])
			    {
			      *sargv = shellLocStr;
			    }
			  if (*sargv
			      && posix_spawnp (&spid, *sargv, NULL, NULL,
					       sargv, envp) != 0)
			    {
			      *sargv = 0;
			    }
			  startCommandStrSize = startCommandStrPos;
			  execute = true;
			  gotonewline = false;
			}
		    }
		  else
		    {
		      appendInternalSrcConst (&newNameStr, &newNameStrSize,
					      &defaulticLoc,
					      &defaulticLocSize,
					      &newNameStrSizeMax);
		      appendInternalSrcConst (&newNameStr, &newNameStrSize,
					      &runshLocStr, &runshLocStrSize,
					      &newNameStrSizeMax);
		      newNameStrSize++;
		      setupDstAppendInternal (&newNameStr, &newNameStrSize,
					      &newNameStrSizeMax);
		      newNameStr[newNameStrSize - 1] = 0;
		      runFile =
			open (newNameStr, O_BINARY | O_RDWR | O_APPEND,
			      S_IRUSR | S_IWUSR);
		      if (runFile == -1)
			{
			  newNameStrSize -=
			    runshLocStrSize - runshDirLocStrSize + 1;
			  newNameStr[newNameStrSize - 1] = 0;
			  mkdim (newNameStr, envp);
			  newNameStr[newNameStrSize - 1] =
			    runshLocStr[runshDirLocStrSize - 1];
			  newNameStrSize -= runshDirLocStrSize;
			  runFile =
			    open (newNameStr,
				  O_BINARY | O_RDWR | O_CREAT | O_APPEND,
				  S_IRUSR | S_IWUSR);
			  write (runFile, runshStr, runshStrSize);
			  for (k = j = 0;
			       j !=
			       sizeof (configStrsNums) /
			       sizeof (*configStrsNums); j++)
			    {
			      appendInternalSrcConst (&newNameStr,
						      &newNameStrSize,
						      &configStrsDir[j],
						      &configStrsDirLen[j],
						      &newNameStrSizeMax);
			      newNameStrSize++;
			      setupDstAppendInternal (&newNameStr,
						      &newNameStrSize,
						      &newNameStrSizeMax);
			      newNameStr[newNameStrSize - 1] = 0;
			      mkdim (newNameStr, envp);
			      newNameStrSize--;
			      appendInternalSrcConst (&newNameStr,
						      &newNameStrSize,
						      &directorySeparator,
						      &directorySeparatorSize,
						      &newNameStrSizeMax);
			      for (l = configStrsNums[j]; l; l--)
				{
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &configStrsLoc[k],
							  &configStrsLocLen
							  [k],
							  &newNameStrSizeMax);
				  newNameStrSize++;
				  setupDstAppendInternal (&newNameStr,
							  &newNameStrSize,
							  &newNameStrSizeMax);
				  newNameStr[newNameStrSize - 1] = 0;
				  fd =
				    open (newNameStr,
					  O_BINARY | O_RDWR | O_APPEND,
					  S_IRUSR | S_IWUSR);
				  execute = fd == -1;
				  if (execute)
				    {
				      fd =
					open (newNameStr,
					      O_BINARY | O_RDWR | O_CREAT |
					      O_APPEND, S_IRUSR | S_IWUSR);
				      execute = fd != -1;
				    }
				  else
				    {
				      fstat (fd, &st);
				      execute = st.st_size == 0;
				    }
				  if (execute)
				    {
				      write (fd, configStrs[k],
					     configStrsLen[k]);
				      close (fd);
				    }
				  newNameStrSize -= configStrsLocLen[k] + 1;
				  k++;
				}
			      newNameStrSize -=
				configStrsDirLen[j] + directorySeparatorSize;
			    }
			  newNameStrSize += runshLocStrSize + 1;
			}
		      newNameStrSize -=
			defaulticLocSize + runshLocStrSize + 1;
		      if (runFile != -1)
			{
			  fstat (runFile, &st);
			  runFileSize = st.st_size;
			  runFileMemAlloc =
			    mmap (0, runFileSize, PROT_READ, MAP_SHARED,
				  runFile, 0);
			  madvise (runFileMemAlloc, runFileSize,
				   MADV_WILLNEED);
			  execute = false;
			  for (j = 0; j != runFileSize; j++)
			    {
			      if (runFileMemAlloc[j] == '\r')
				{
				  j += j + 1 != runFileSize;
				}
			      if (!gotonewline)
				{
				  runFileLine++;
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &defaulticLoc,
							  &defaulticLocSize,
							  &newNameStrSizeMax);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &runconfigLocStr,
							  &runconfigLocStrSize,
							  &newNameStrSizeMax);
				  lPos1 = runFileLine;
				  for (lSize1 = 0; lPos1; lSize1++)
				    {
				      newNameStrSize++;
				      setupDstAppendInternal (&newNameStr,
							      &newNameStrSize,
							      &newNameStrSizeMax);
				      newNameStr[newNameStrSize - 1] =
					'0' + lPos1 % 10;
				      lPos1 /= 10;
				    }
				  for (; lPos1 != lSize1 >> 1; lPos1++)
				    {
				      k =
					newNameStr[newNameStrSize + lPos1 -
						   lSize1];
				      newNameStr[newNameStrSize + lPos1 -
						 lSize1] =
					newNameStr[newNameStrSize - 1 -
						   lPos1];
				      newNameStr[newNameStrSize - lPos1 - 1] =
					k;
				    }
				  newNameStrSize++;
				  setupDstAppendInternal (&newNameStr,
							  &newNameStrSize,
							  &newNameStrSizeMax);
				  newNameStr[newNameStrSize - 1] = 0;
				  runconfig =
				    open (newNameStr,
					  O_BINARY | O_RDWR | O_APPEND,
					  S_IRUSR | S_IWUSR);
				  runconfigMemAllocPos = 0;
				  fstat (runconfig, &st);
				  runconfigSize = st.st_size;
				  runconfigMemAlloc =
				    mmap (0, runconfigSize, PROT_READ,
					  MAP_SHARED, runconfig, 0);
				  nQuotes = 0;
				  runFileMemAllocPos = j;
				  execute = j != runFileSize;
				  while (!nQuotes)
				    {
				      for (;
					   j != runFileSize
					   && runFileMemAlloc[j] == ' '; j++);
				      for (;
					   j != runFileSize
					   && runFileMemAlloc[j] == '\"'; j++)
					{
					  nQuotes++;
					}
				      execute = j != runFileSize;
				      execute = execute
					&& runFileMemAlloc[j] == 'c';
				      j += execute;
				      if (execute)
					{
					  for (;
					       j != runFileSize
					       && runFileMemAlloc[j] == '\"';
					       j++)
					    {
					      nQuotes++;
					    }
					}
				      execute = execute && j != runFileSize;
				      execute = execute
					&& runFileMemAlloc[j] == 'd';
				      if (!nQuotes)
					{
					  if (runFileMemAlloc[j] == '=')
					    {
					      break;
					    }
					  else
					    {
					      for (;
						   j != runFileSize
						   && runFileMemAlloc[j] !=
						   '='
						   && runFileMemAlloc[j] !=
						   ' '
						   && runFileMemAlloc[j] !=
						   '\"'
						   &&
						   !isnewlinechar
						   (runFileMemAlloc[j]); j++);
					      if (j == runFileSize
						  || runFileMemAlloc[j] == ' '
						  || runFileMemAlloc[j] ==
						  '\"'
						  ||
						  isnewlinechar
						  (runFileMemAlloc[j]))
						{
						  break;
						}
					      if (runFileMemAlloc[j] == '=')
						{
						  for (;
						       j != runFileSize
						       && runFileMemAlloc[j]
						       != ' '
						       &&
						       !isnewlinechar
						       (runFileMemAlloc[j]);
						       j++)
						    {
						      if (runFileMemAlloc[j]
							  == '\"')
							{
							  for (;
							       j !=
							       runFileSize
							       &&
							       runFileMemAlloc
							       [j] != '\"';
							       j++);
							}
						    }
						  nQuotes = j == runFileSize;
						}
					    }
					}
				    }
				  j += execute && j != runFileSize
				    && runFileMemAlloc[j] == 'd';
				  if (execute)
				    {
				      for (;
					   j != runFileSize
					   && runFileMemAlloc[j] == '\"'; j++)
					{
					  nQuotes++;
					}
				    }
				  execute = execute && 1 - (nQuotes & 1);
				  execute = execute && j != runFileSize;
				  if (execute)
				    {
				      for (;
					   j != runFileSize
					   && runFileMemAlloc[j] == ' '; j++);
				    }
				  execute = 1 - execute;
				  execute = execute && j != runFileSize;
				  if (execute)
				    {
				      j = runFileMemAllocPos;
				      appendInternalSrcConst
					(&startCommandStr,
					 &startCommandStrSize,
					 &commandSucessStr,
					 &commandSucessStrSize,
					 &startCommandStrSizeMax);
				    }
				  else
				    {
				      while (j != runFileSize
					     &&
					     !isnewlinechar (runFileMemAlloc
							     [j])
					     && !(runFileMemAlloc[j] == '\r'
						  && j + 1 != runFileSize
						  &&
						  isnewlinechar
						  (runFileMemAlloc[j + 1])))
					{
					  for (;
					       j != runFileSize
					       && (runFileMemAlloc[j] == '/'
						   || runFileMemAlloc[j] ==
						   '\"'); j++)
					    {
					      nQuotes +=
						runFileMemAlloc[j] == '\"';
					    }
					  for (;
					       j != runFileSize
					       && (runFileMemAlloc[j] == '\\'
						   || runFileMemAlloc[j] ==
						   '\"'); j++)
					    {
					      nQuotes +=
						runFileMemAlloc[j] == '\"';
					    }
					  if (1 - (nQuotes & 1)
					      && j != runFileSize
					      && runFileMemAlloc[j] == ' ')
					    {
					      break;
					    }
					  if (runFileMemAlloc[j] == '.')
					    {
					      if (j + 1 != runFileSize
						  && runFileMemAlloc[j + 1] ==
						  '.')
						{
						  if (j + 2 != runFileSize
						      &&
						      (runFileMemAlloc[j + 2]
						       == '/'
						       || runFileMemAlloc[j +
									  2]
						       == '\\'
						       ||
						       isnewlinechar
						       (runFileMemAlloc
							[j + 2])
						       ||
						       (runFileMemAlloc[j + 2]
							== '\r'
							&& j + 3 !=
							runFileSize
							&&
							isnewlinechar
							(runFileMemAlloc
							 [j + 3]))))
						    {
						      for (;
							   startCommandStrSize
							   != cdStrSize
							   &&
							   startCommandStr
							   [startCommandStrSize]
							   != '/'
							   &&
							   startCommandStr
							   [startCommandStrSize]
							   != '\\';
							   startCommandStrSize--);
						      execute = true;
						    }
						}
					      else if (j + 1 != runFileSize
						       &&
						       (runFileMemAlloc[j + 1]
							== '/'
							|| runFileMemAlloc[j +
									   1]
							== '\\'
							||
							isnewlinechar
							(runFileMemAlloc
							 [j + 1])
							||
							(runFileMemAlloc
							 [j + 1] == '\r'
							 && j + 2 !=
							 runFileSize
							 &&
							 isnewlinechar
							 (runFileMemAlloc
							  [j + 2]))))
						{
						  execute = true;
						}
					    }
					  if (execute)
					    {
					      for (;
						   j != runFileSize
						   && runFileMemAlloc[j] ==
						   '.'; j++);
					    }
					  else
					    {
					      appendInternalSrcConst
						(&startCommandStr,
						 &startCommandStrSize,
						 &directorySeparator,
						 &directorySeparatorSize,
						 &startCommandStrSizeMax);
					      for (;
						   j != runFileSize
						   && !(1 - (nQuotes & 1)
							&& runFileMemAlloc[j]
							== ' ')
						   && runFileMemAlloc[j] !=
						   '/'
						   && runFileMemAlloc[j] !=
						   '\\'
						   &&
						   !isnewlinechar
						   (runFileMemAlloc[j])
						   && !(runFileMemAlloc[j] ==
							'\r'
							&& j + 1 !=
							runFileSize
							&&
							isnewlinechar
							(runFileMemAlloc
							 [j + 1])); j++)
						{
						  if (runFileMemAlloc[j] ==
						      '\"')
						    {
						      nQuotes++;
						    }
						  else
						    {
						      startCommandStrSize++;
						      setupDstAppendInternal
							(&startCommandStr,
							 &startCommandStrSize,
							 &startCommandStrSizeMax);
						      startCommandStr
							[startCommandStrSize -
							 1] =
							runFileMemAlloc[j];
						    }
						}
					    }
					  execute = false;
					}
				      startCommandStrPos =
					startCommandStrSize;
				    }
				  newNameStrSize -=
				    runconfigLocStrSize + lSize1 + 1;
				  configMemAlloc = runconfigMemAlloc;
				  if (runconfigMemAlloc != MAP_FAILED)
				    {
				      madvise (runconfigMemAlloc,
					       runconfigSize, MADV_WILLNEED);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &configLocStr,
							      &configLocStrSize,
							      &newNameStrSizeMax);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &directorySeparator,
							      &directorySeparatorSize,
							      &newNameStrSizeMax);
				      appendInternal (&newNameStr,
						      &newNameStrSize,
						      &runconfigMemAlloc,
						      &runconfigSize,
						      &newNameStrSizeMax);
				      newNameStrSize++;
				      setupDstAppendInternal (&newNameStr,
							      &newNameStrSize,
							      &newNameStrSizeMax);
				      newNameStr[newNameStrSize - 1] = 0;
				      newNameStrSize -=
					configLocStrSize +
					directorySeparatorSize +
					runconfigSize + 1;
				      config =
					open (newNameStr,
					      O_BINARY | O_RDWR | O_APPEND,
					      S_IRUSR | S_IWUSR);
				      configMemAllocPos = 0;
				      fstat (config, &st);
				      configSize = st.st_size;
				      configMemAlloc =
					mmap (0, configSize, PROT_READ,
					      MAP_SHARED, config, 0);
				      if (configMemAlloc != MAP_FAILED)
					{
					  madvise (configMemAlloc, configSize,
						   MADV_WILLNEED);
					}
				    }
				  newNameStrSize -= defaulticLocSize;
				  gotonewline = true;
				}
			      if (isnewlinechar (runFileMemAlloc[j]))
				{
				  gotonewline = false;
				}
			      if (execute && gotonewline)
				{
				  startCommandStrSize++;
				  setupDstAppendInternal (&startCommandStr,
							  &startCommandStrSize,
							  &startCommandStrSizeMax);
				  startCommandStr[startCommandStrSize - 1] =
				    runFileMemAlloc[j];
				}
			      else if (execute)
				{
				  if (configMemAlloc != MAP_FAILED)
				    {
				      for (quoteType = nQuotes =
					   configMemAllocPos = 0;
					   configMemAllocPos != configSize;
					   configMemAllocPos++)
					{
					  if (!gotonewline)
					    {
					      startCommandStrSize++;
					      setupDstAppendInternal
						(&startCommandStr,
						 &startCommandStrSize,
						 &startCommandStrSizeMax);
					      startCommandStr
						[startCommandStrSize - 1] =
						' ';
					      execute = gotonewline = true;
					    }
					  if (isnewlinechar
					      (configMemAlloc
					       [configMemAllocPos]))
					    {
					      gotonewline = false;
					    }
					  if (configMemAlloc
					      [configMemAllocPos] == '\r'
					      && configMemAllocPos + 1 !=
					      configSize
					      &&
					      isnewlinechar (configMemAlloc
							     [configMemAllocPos
							      + 1]))
					    {
					      configMemAllocPos++;
					      gotonewline = false;
					    }
					  if (execute
					      &&
					      configMemAlloc
					      [configMemAllocPos] == '#'
					      && (!configMemAllocPos
						  ||
						  configMemAlloc
						  [configMemAllocPos - 1] ==
						  ' '
						  ||
						  isnewlinechar
						  (configMemAlloc
						   [configMemAllocPos - 1]))
					      && 1 - (nQuotes & 1))
					    {
					      execute = false;
					    }
					  if ((execute
					       &&
					       configMemAlloc
					       [configMemAllocPos] == '\"')
					      ||
					      configMemAlloc
					      [configMemAllocPos] == '\'')
					    {
					      if (1 - (nQuotes & 1))
						{
						  quoteType =
						    configMemAlloc
						    [configMemAllocPos];
						}
					      nQuotes +=
						quoteType ==
						configMemAlloc
						[configMemAllocPos];
					    }
					  if (gotonewline & execute)
					    {
					      startCommandStrSize++;
					      setupDstAppendInternal
						(&startCommandStr,
						 &startCommandStrSize,
						 &startCommandStrSizeMax);
					      startCommandStr
						[startCommandStrSize - 1] =
						configMemAlloc
						[configMemAllocPos];
					    }
					}
				    }
				  if (runconfigMemAlloc != MAP_FAILED)
				    {
				      madvise (runconfigMemAlloc,
					       runconfigSize, MADV_WILLNEED);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &additionalicLoc,
							      &additionalicLocSize,
							      &newNameStrSizeMax);
				      appendInternal (&newNameStr,
						      &newNameStrSize,
						      &argv[i], &argvSize[i],
						      &newNameStrSizeMax);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &directorySeparator,
							      &directorySeparatorSize,
							      &newNameStrSizeMax);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &configLocStr,
							      &configLocStrSize,
							      &newNameStrSizeMax);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &directorySeparator,
							      &directorySeparatorSize,
							      &newNameStrSizeMax);
				      appendInternal (&newNameStr,
						      &newNameStrSize,
						      &runconfigMemAlloc,
						      &runconfigSize,
						      &newNameStrSizeMax);
				      newNameStrSize++;
				      setupDstAppendInternal (&newNameStr,
							      &newNameStrSize,
							      &newNameStrSizeMax);
				      newNameStr[newNameStrSize - 1] = 0;
				      newNameStrSize -=
					additionalicLocSize + argvSize[i] +
					directorySeparatorSize +
					configLocStrSize +
					directorySeparatorSize +
					runconfigSize + 1;
				      munmap (runconfigMemAlloc,
					      runconfigSize);
				      config =
					open (newNameStr,
					      O_BINARY | O_RDWR | O_APPEND,
					      S_IRUSR | S_IWUSR);
				      configMemAllocPos = 0;
				      fstat (config, &st);
				      configSize = st.st_size;
				      configMemAlloc =
					mmap (0, configSize, PROT_READ,
					      MAP_SHARED, config, 0);
				      if (configMemAlloc != MAP_FAILED)
					{
					  madvise (configMemAlloc, configSize,
						   MADV_WILLNEED);
					}
				    }
				  gotonewline = false;
				  if (configMemAlloc != MAP_FAILED)
				    {
				      for (quoteType = nQuotes =
					   configMemAllocPos = 0;
					   configMemAllocPos != configSize;
					   configMemAllocPos++)
					{
					  if (!gotonewline)
					    {
					      startCommandStrSize++;
					      setupDstAppendInternal
						(&startCommandStr,
						 &startCommandStrSize,
						 &startCommandStrSizeMax);
					      startCommandStr
						[startCommandStrSize - 1] =
						' ';
					      execute = gotonewline = true;
					    }
					  if (isnewlinechar
					      (configMemAlloc
					       [configMemAllocPos]))
					    {
					      gotonewline = false;
					    }
					  if (configMemAlloc
					      [configMemAllocPos] == '\r'
					      && configMemAllocPos + 1 !=
					      configSize
					      &&
					      isnewlinechar (configMemAlloc
							     [configMemAllocPos
							      + 1]))
					    {
					      configMemAllocPos++;
					      gotonewline = false;
					    }
					  if (execute
					      &&
					      configMemAlloc
					      [configMemAllocPos] == '#'
					      && (!configMemAllocPos
						  ||
						  configMemAlloc
						  [configMemAllocPos - 1] ==
						  ' '
						  ||
						  isnewlinechar
						  (configMemAlloc
						   [configMemAllocPos - 1]))
					      && 1 - (nQuotes & 1))
					    {
					      execute = false;
					    }
					  if ((execute
					       &&
					       configMemAlloc
					       [configMemAllocPos] == '\"')
					      ||
					      configMemAlloc
					      [configMemAllocPos] == '\'')
					    {
					      if (1 - (nQuotes & 1))
						{
						  quoteType =
						    configMemAlloc
						    [configMemAllocPos];
						}
					      nQuotes +=
						quoteType ==
						configMemAlloc
						[configMemAllocPos];
					    }
					  if (gotonewline & execute)
					    {
					      startCommandStrSize++;
					      setupDstAppendInternal
						(&startCommandStr,
						 &startCommandStrSize,
						 &startCommandStrSizeMax);
					      startCommandStr
						[startCommandStrSize - 1] =
						configMemAlloc
						[configMemAllocPos];
					    }
					}
				    }
				  startCommandStrSize++;
				  setupDstAppendInternal (&startCommandStr,
							  &startCommandStrSize,
							  &startCommandStrSizeMax);
				  startCommandStr[startCommandStrSize - 1] =
				    0;
				  sargv[2] = startCommandStr;
				  if (*sargv)
				    {
				      waitid (P_PID, spid, &ssiginfo,
					      WEXITED);
				    }
				  if (sargv[2])
				    {
				      *sargv = shellLocStr;
				    }
				  if (*sargv
				      && posix_spawnp (&spid, *sargv, NULL,
						       NULL, sargv,
						       envp) != 0)
				    {
				      *sargv = 0;
				    }
				  startCommandStrSize = startCommandStrPos;
				  execute = true;
				  gotonewline = false;
				}
			    }
			  if (execute)
			    {
			      if (configMemAlloc != MAP_FAILED)
				{
				  for (quoteType = nQuotes =
				       configMemAllocPos = 0;
				       configMemAllocPos != configSize;
				       configMemAllocPos++)
				    {
				      if (!gotonewline)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] = ' ';
					  execute = gotonewline = true;
					}
				      if (isnewlinechar
					  (configMemAlloc[configMemAllocPos]))
					{
					  gotonewline = false;
					}
				      if (configMemAlloc[configMemAllocPos] ==
					  '\r'
					  && configMemAllocPos + 1 !=
					  configSize
					  &&
					  isnewlinechar (configMemAlloc
							 [configMemAllocPos +
							  1]))
					{
					  configMemAllocPos++;
					  gotonewline = false;
					}
				      if (execute
					  && configMemAlloc[configMemAllocPos]
					  == '#' && (!configMemAllocPos
						     ||
						     configMemAlloc
						     [configMemAllocPos -
						      1] == ' '
						     ||
						     isnewlinechar
						     (configMemAlloc
						      [configMemAllocPos -
						       1]))
					  && 1 - (nQuotes & 1))
					{
					  execute = false;
					}
				      if ((execute
					   &&
					   configMemAlloc[configMemAllocPos]
					   == '\"')
					  || configMemAlloc[configMemAllocPos]
					  == '\'')
					{
					  if (1 - (nQuotes & 1))
					    {
					      quoteType =
						configMemAlloc
						[configMemAllocPos];
					    }
					  nQuotes +=
					    quoteType ==
					    configMemAlloc[configMemAllocPos];
					}
				      if (gotonewline & execute)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] =
					    configMemAlloc[configMemAllocPos];
					}
				    }
				}
			      if (runconfigMemAlloc != MAP_FAILED)
				{
				  madvise (runconfigMemAlloc, runconfigSize,
					   MADV_WILLNEED);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &additionalicLoc,
							  &additionalicLocSize,
							  &newNameStrSizeMax);
				  appendInternal (&newNameStr,
						  &newNameStrSize, &argv[i],
						  &argvSize[i],
						  &newNameStrSizeMax);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &directorySeparator,
							  &directorySeparatorSize,
							  &newNameStrSizeMax);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &configLocStr,
							  &configLocStrSize,
							  &newNameStrSizeMax);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &directorySeparator,
							  &directorySeparatorSize,
							  &newNameStrSizeMax);
				  appendInternal (&newNameStr,
						  &newNameStrSize,
						  &runconfigMemAlloc,
						  &runconfigSize,
						  &newNameStrSizeMax);
				  newNameStrSize++;
				  setupDstAppendInternal (&newNameStr,
							  &newNameStrSize,
							  &newNameStrSizeMax);
				  newNameStr[newNameStrSize - 1] = 0;
				  newNameStrSize -=
				    additionalicLocSize + argvSize[i] +
				    directorySeparatorSize +
				    configLocStrSize +
				    directorySeparatorSize + runconfigSize +
				    1;
				  munmap (runconfigMemAlloc, runconfigSize);
				  config =
				    open (newNameStr,
					  O_BINARY | O_RDWR | O_APPEND,
					  S_IRUSR | S_IWUSR);
				  configMemAllocPos = 0;
				  fstat (config, &st);
				  configSize = st.st_size;
				  configMemAlloc =
				    mmap (0, configSize, PROT_READ,
					  MAP_SHARED, config, 0);
				  if (configMemAlloc != MAP_FAILED)
				    {
				      madvise (configMemAlloc, configSize,
					       MADV_WILLNEED);
				    }
				}
			      gotonewline = false;
			      if (configMemAlloc != MAP_FAILED)
				{
				  for (quoteType = nQuotes =
				       configMemAllocPos = 0;
				       configMemAllocPos != configSize;
				       configMemAllocPos++)
				    {
				      if (!gotonewline)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] = ' ';
					  execute = gotonewline = true;
					}
				      if (isnewlinechar
					  (configMemAlloc[configMemAllocPos]))
					{
					  gotonewline = false;
					}
				      if (configMemAlloc[configMemAllocPos] ==
					  '\r'
					  && configMemAllocPos + 1 !=
					  configSize
					  &&
					  isnewlinechar (configMemAlloc
							 [configMemAllocPos +
							  1]))
					{
					  configMemAllocPos++;
					  gotonewline = false;
					}
				      if (execute
					  && configMemAlloc[configMemAllocPos]
					  == '#' && (!configMemAllocPos
						     ||
						     configMemAlloc
						     [configMemAllocPos -
						      1] == ' '
						     ||
						     isnewlinechar
						     (configMemAlloc
						      [configMemAllocPos -
						       1]))
					  && 1 - (nQuotes & 1))
					{
					  execute = false;
					}
				      if ((execute
					   &&
					   configMemAlloc[configMemAllocPos]
					   == '\"')
					  || configMemAlloc[configMemAllocPos]
					  == '\'')
					{
					  if (1 - (nQuotes & 1))
					    {
					      quoteType =
						configMemAlloc
						[configMemAllocPos];
					    }
					  nQuotes +=
					    quoteType ==
					    configMemAlloc[configMemAllocPos];
					}
				      if (gotonewline & execute)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] =
					    configMemAlloc[configMemAllocPos];
					}
				    }
				}
			      startCommandStrSize++;
			      setupDstAppendInternal (&startCommandStr,
						      &startCommandStrSize,
						      &startCommandStrSizeMax);
			      startCommandStr[startCommandStrSize - 1] = 0;
			      sargv[2] = startCommandStr;
			      if (*sargv)
				{
				  waitid (P_PID, spid, &ssiginfo, WEXITED);
				}
			      if (sargv[2])
				{
				  *sargv = shellLocStr;
				}
			      if (*sargv
				  && posix_spawnp (&spid, *sargv, NULL, NULL,
						   sargv, envp) != 0)
				{
				  *sargv = 0;
				}
			      startCommandStrSize = startCommandStrPos;
			      execute = true;
			      gotonewline = false;
			    }
			}
		    }
		  startCommandStrSize = 0;
		  installDirNameSize--;
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  &installDir, &installDirNameSize,
				  &startCommandStrSizeMax);
		  installDirNameSize++;
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &archivesLoc,
					  &archivesLocSize,
					  &startCommandStrSizeMax);
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  &argv[i], &argvSize[i],
				  &startCommandStrSizeMax);
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize,
					  &installLocStr, &installLocStrSize,
					  &startCommandStrSizeMax);
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &infoLocStr,
					  &infoLocStrSize,
					  &startCommandStrSizeMax);
		  startCommandStr[startCommandStrSize] = 0;
		  mkdim (startCommandStr, envp);
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  &argv[i], &argvSize[i],
				  &startCommandStrSizeMax);
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize,
					  &listFileExtensionStr,
					  &listFileExtensionStrSize,
					  &startCommandStrSizeMax);
		  startCommandStr[startCommandStrSize] = 0;
		  newListFile =
		    open (startCommandStr, O_BINARY | O_RDWR | O_CREAT);
		  startCommandStrSize -=
		    listFileExtensionStrSize + argvSize[i] + infoLocStrSize;
		  startCommandStr[startCommandStrSize] = 0;
		  startILFE =
		    (struct internalListFileEntry *)
		    malloc (sizeof (*startILFE));
		  startILFE->fileLoc = ".";
		  startILFE->fileLocSize = strlen (startILFE->fileLoc) + 1;
		  startILFE->directoryLoc = "/";
		  startILFE->directoryLocSize =
		    strlen (startILFE->directoryLoc);
		  prevdirectory = startILFE;
		  startILFE->prevdirectory = prevdirectory;
		  startILFE->isdirectory = 1;
		  startILFE->next =
		    (struct internalListFileEntry *)
		    malloc (sizeof (*currILFE));
		  *startILFE->next = *startILFE;
		  previDS =
		    (struct internalDirectoryScan *)
		    malloc (sizeof (*previDS));
		  previDS->iDirectory = startILFE->next;
		  previDS->prevdirectory = prevdirectory;
		  previDS->prev = 0;
		  currILFE = startILFE;
		  curriDS = previDS;
		  dirp = opendir (startCommandStr);
		  newListFileSize =
		    currILFE->fileLocSize + currILFE->directoryLocSize;
		  if (*sargv)
		    {
		      waitid (P_PID, spid, &ssiginfo, WEXITED);
		    }
		  *sargv = "test";
		  sargv[1] = "-L";
		  sargv[3] = 0;
		  if (dirp)
		    {
		      curriDS->dirp = dirp;
		      while (curriDS)
			{
			  prevdirectory = curriDS->prevdirectory;
			  previDS = curriDS->prev;
			  dirp = curriDS->dirp;
			  while ((dir = readdir (dirp)) != NULL)
			    {
			      if ((!
				   (dir->d_name[0] == '.'
				    && dir->d_name[1] == 0))
				  &&
				  (!(dir->d_name[0] == '.'
				     && dir->d_name[1] == '.'
				     && dir->d_name[2] == 0)))
				{
				  currILFE->next->next =
				    (struct internalListFileEntry *)
				    malloc (sizeof (*currILFE));
				  currILFE = currILFE->next;
				  currILFE->fileLocSize =
				    strlen (dir->d_name) + 1;
				  currILFE->fileLoc =
				    (char *) malloc (currILFE->fileLocSize *
						     sizeof (char));
				  currILFE->fileLoc[currILFE->fileLocSize -
						    1] = 0;
				  currILFE->directoryLoc =
				    curriDS->iDirectory->directoryLoc;
				  currILFE->directoryLocSize =
				    curriDS->iDirectory->directoryLocSize;
				  currILFE->prevdirectory = prevdirectory;
				  currILFE->isdirectory = 0;
				  memcpy (currILFE->fileLoc, dir->d_name,
					  currILFE->fileLocSize *
					  sizeof (char));
				  newListFileSize +=
				    currILFE->fileLocSize +
				    currILFE->directoryLocSize;
				  appendInternal (&startCommandStr,
						  &startCommandStrSize,
						  &currILFE->directoryLoc,
						  &currILFE->directoryLocSize,
						  &startCommandStrSizeMax);
				  appendInternal (&startCommandStr,
						  &startCommandStrSize,
						  &currILFE->fileLoc,
						  &currILFE->fileLocSize,
						  &startCommandStrSizeMax);
				  startCommandStr[startCommandStrSize - 1] =
				    0;
				  sargv[2] = startCommandStr;
				  if (sargv[2]
				      && !posix_spawnp (&spid, *sargv, NULL,
							NULL, sargv, envp))
				    {
				      if (waitid
					  (P_PID, spid, &ssiginfo,
					   WEXITED) != -1)
					{
					  if (ssiginfo.si_signo != SIGCHLD
					      || ssiginfo.si_code !=
					      CLD_EXITED)
					    {
					      ssiginfo.si_status = 1;
					    }
					}
				      else
					{
					  ssiginfo.si_status = 1;
					}
				    }
				  else
				    {
				      ssiginfo.si_status = 1;
				    }
				  if (ssiginfo.si_status)
				    {
				      dirp = opendir (startCommandStr);
				      if (dirp)
					{
					  currILFE->isdirectory = 1;
					  prevdirectory = currILFE;
					  previDS = curriDS;
					  curriDS =
					    (struct internalDirectoryScan *)
					    malloc (sizeof (*previDS));
					  curriDS->prevdirectory =
					    prevdirectory;
					  curriDS->iDirectory =
					    currILFE->next;
					  curriDS->iDirectory->
					    directoryLocSize =
					    currILFE->directoryLocSize +
					    currILFE->fileLocSize;
					  curriDS->iDirectory->directoryLoc =
					    (char *) malloc (curriDS->
							     iDirectory->
							     directoryLocSize);
					  memcpy (curriDS->iDirectory->
						  directoryLoc,
						  currILFE->directoryLoc,
						  currILFE->directoryLocSize);
					  memcpy (curriDS->iDirectory->
						  directoryLoc +
						  currILFE->directoryLocSize,
						  currILFE->fileLoc,
						  currILFE->fileLocSize);
					  curriDS->iDirectory->
					    directoryLoc[curriDS->iDirectory->
							 directoryLocSize -
							 1] = '/';
					  curriDS->dirp = dirp;
					  curriDS->prev = previDS;
					}
				      dirp = curriDS->dirp;
				    }
				  startCommandStrSize -=
				    currILFE->directoryLocSize;
				  startCommandStrSize -=
				    currILFE->fileLocSize;
				}
			    }
			  closedir (dirp);
			  previDS = curriDS->prev;
			  free (curriDS);
			  curriDS = previDS;
			}
		    }
		  currILFE->next = 0;
		  newListFileMemAlloc = (char *) malloc (newListFileSize);
		  newListFileMemAllocPos = 0;
		  currILFE = startILFE;
		  while (currILFE)
		    {
		      memcpy (newListFileMemAlloc + newListFileMemAllocPos,
			      currILFE->directoryLoc,
			      currILFE->directoryLocSize);
		      newListFileMemAllocPos += currILFE->directoryLocSize;
		      newListFileMemAlloc[newListFileMemAllocPos - 1] = '/';
		      memcpy (newListFileMemAlloc + newListFileMemAllocPos,
			      currILFE->fileLoc, currILFE->fileLocSize);
		      newListFileMemAllocPos += currILFE->fileLocSize;
		      newListFileMemAlloc[newListFileMemAllocPos - 1] = '\n';
		      currILFE = currILFE->next;
		    }
		  write (newListFile, newListFileMemAlloc,
			 newListFileMemAllocPos);
		  close (newListFile);
		  free (newListFileMemAlloc);
		  newNameStrSize = 0;
		  installDirNameSize -= 2;
		  appendInternal (&newNameStr, &newNameStrSize, &installDir,
				  &installDirNameSize, &newNameStrSizeMax);
		  installDirNameSize += 2;
		  currILFE = startILFE;
		  currILFE = currILFE->next;
		  while (currILFE)
		    {
		      if (currILFE->prevdirectory
			  && currILFE->prevdirectory->prevdirectory)
			{
			  appendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &currILFE->directoryLoc,
					  &currILFE->directoryLocSize,
					  &startCommandStrSizeMax);
			  appendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &currILFE->fileLoc,
					  &currILFE->fileLocSize,
					  &startCommandStrSizeMax);
			  appendInternalSrcConst (&startCommandStr,
						  &startCommandStrSize,
						  &checkIfSymbolicLinkEndStr,
						  &checkIfSymbolicLinkEndStrSize,
						  &startCommandStrSizeMax);
			  appendInternal (&newNameStr, &newNameStrSize,
					  &currILFE->directoryLoc,
					  &currILFE->directoryLocSize,
					  &newNameStrSizeMax);
			  appendInternal (&newNameStr, &newNameStrSize,
					  &currILFE->fileLoc,
					  &currILFE->fileLocSize,
					  &newNameStrSizeMax);
			  appendInternalSrcConst (&newNameStr,
						  &newNameStrSize,
						  &checkIfSymbolicLinkEndStr,
						  &checkIfSymbolicLinkEndStrSize,
						  &newNameStrSizeMax);
			  startCommandStr[startCommandStrSize - 2] = 0;
			  if (!rename (startCommandStr, newNameStr))
			    {
			      currILFE->prevdirectory = 0;
			    }
			  newNameStrSize -= currILFE->directoryLocSize;
			  newNameStrSize -= currILFE->fileLocSize;
			  newNameStrSize -= checkIfSymbolicLinkEndStrSize;
			  startCommandStrSize -= currILFE->directoryLocSize;
			  startCommandStrSize -= currILFE->fileLocSize;
			  startCommandStrSize -=
			    checkIfSymbolicLinkEndStrSize;
			}
		      else
			{
			  currILFE->prevdirectory = 0;
			}
		      currILFE = currILFE->next;
		    }
		  tmpNewNameStrSize = 0;
		  currILFE = startILFE;
		  currILFE = currILFE->next;
		  while (currILFE)
		    {
		      if (!currILFE->isdirectory)
			{
			  if (currILFE->prevdirectory
			      && currILFE->prevdirectory->prevdirectory)
			    {
			      if (!tmpNewNameStrSize)
				{
				  installDirNameSize -= 2;
				  appendInternal (&tmpNewNameStr,
						  &tmpNewNameStrSize,
						  &installDir,
						  &installDirNameSize,
						  &tmpNewNameStrSizeMax);
				  installDirNameSize += 2;
				}
			      appendInternal (&startCommandStr,
					      &startCommandStrSize,
					      &currILFE->directoryLoc,
					      &currILFE->directoryLocSize,
					      &startCommandStrSizeMax);
			      appendInternal (&startCommandStr,
					      &startCommandStrSize,
					      &currILFE->fileLoc,
					      &currILFE->fileLocSize,
					      &startCommandStrSizeMax);
			      appendInternal (&newNameStr, &newNameStrSize,
					      &currILFE->directoryLoc,
					      &currILFE->directoryLocSize,
					      &newNameStrSizeMax);
			      appendInternal (&newNameStr, &newNameStrSize,
					      &currILFE->fileLoc,
					      &currILFE->fileLocSize,
					      &newNameStrSizeMax);
			      newNameStr += installDirNameSize - 2;
			      newNameStrSize -= installDirNameSize - 2;
			      appendInternal (&tmpNewNameStr,
					      &tmpNewNameStrSize, &newNameStr,
					      &newNameStrSize,
					      &tmpNewNameStrSizeMax);
			      newNameStr -= installDirNameSize - 2;
			      newNameStrSize += installDirNameSize - 2;
			      tmpNewNameStrSize--;
			      appendInternalSrcConst (&tmpNewNameStr,
						      &tmpNewNameStrSize,
						      &tmpFileExtensionStr,
						      &tmpFileExtensionStrSize,
						      &tmpNewNameStrSizeMax);
			      tmpNewNameStrSize++;
			      setupDstAppendInternal (&tmpNewNameStr,
						      &tmpNewNameStrSize,
						      &tmpNewNameStrSizeMax);
			      tmpNewNameStr[tmpNewNameStrSize - 1] = 0;
			      if (!stat (tmpNewNameStr, &st))
				{
				  tmpNewNameStrSize -=
				    tmpFileExtensionStrSize;
				  tmpNewNumberPos = tmpNewNameStrSize;
				  tmpNewNumberPosMin = tmpNewNumberPos;
				  tmpNewNumberPosMax = tmpNewNumberPos;
				  tmpNewNameStr[tmpNewNumberPos] = '1';
				  tmpNewNameStrSize++;
				  appendInternalSrcConst (&tmpNewNameStr,
							  &tmpNewNameStrSize,
							  &tmpFileExtensionStr,
							  &tmpFileExtensionStrSize,
							  &tmpNewNameStrSizeMax);
				  tmpNewNameStrSize++;
				  setupDstAppendInternal (&tmpNewNameStr,
							  &tmpNewNameStrSize,
							  &tmpNewNameStrSizeMax);
				  tmpNewNameStr[tmpNewNameStrSize - 1] = 0;
				  while (!stat (tmpNewNameStr, &st))
				    {
				      if (tmpNewNameStr[tmpNewNumberPos] ==
					  '9')
					{
					  if (tmpNewNumberPos ==
					      tmpNewNumberPosMin)
					    {
					      tmpNewNumberPosMax++;
					      tmpNewNameStrSize =
						tmpNewNumberPosMax + 1;
					      tmpNewNameStr[tmpNewNumberPos] =
						'1';
					      for (tmpNewNumberPos++;
						   tmpNewNumberPos !=
						   tmpNewNumberPosMax;
						   tmpNewNumberPos++)
						{
						  tmpNewNameStr
						    [tmpNewNumberPos] = '0';
						}
					      tmpNewNameStr
						[tmpNewNumberPosMax] = '0';
					      tmpNewNumberPos =
						tmpNewNumberPosMax;
					      appendInternalSrcConst
						(&tmpNewNameStr,
						 &tmpNewNameStrSize,
						 &tmpFileExtensionStr,
						 &tmpFileExtensionStrSize,
						 &tmpNewNameStrSizeMax);
					      tmpNewNameStrSize++;
					      setupDstAppendInternal
						(&tmpNewNameStr,
						 &tmpNewNameStrSize,
						 &tmpNewNameStrSizeMax);
					      tmpNewNameStr[tmpNewNameStrSize
							    - 1] = 0;
					    }
					  else
					    {
					      tmpNewNameStr[tmpNewNumberPos] =
						'0';
					      tmpNewNumberPos--;
					    }
					}
				      else
					{
					  tmpNewNameStr[tmpNewNumberPos]++;
					  if (tmpNewNumberPos !=
					      tmpNewNumberPosMax)
					    {
					      tmpNewNumberPos =
						tmpNewNumberPosMax;
					    }
					}
				    }
				}
			      rename (newNameStr, tmpNewNameStr);
			      if (!rename (startCommandStr, newNameStr))
				{
				  if (remove (tmpNewNameStr))
				    {
				      chmod (tmpNewNameStr, S_IWUSR);
				      remove (tmpNewNameStr);
				    }
				  currILFE->prevdirectory = 0;
				}
			      tmpNewNameStrSize = installDirNameSize - 2;
			      newNameStrSize -= currILFE->directoryLocSize;
			      newNameStrSize -= currILFE->fileLocSize;
			      startCommandStrSize -=
				currILFE->directoryLocSize;
			      startCommandStrSize -= currILFE->fileLocSize;
			    }
			  else
			    {
			      currILFE->prevdirectory = 0;
			    }
			}
		      currILFE = currILFE->next;
		    }
		  startCommandStrSize -= installLocStrSize;
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &gitLocStr,
					  &gitLocStrSize,
					  &startCommandStrSizeMax);
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &HEADLocStr,
					  &HEADLocStrSize,
					  &startCommandStrSizeMax);
		  startCommandStrSize++;
		  setupDstAppendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &startCommandStrSizeMax);
		  startCommandStr[startCommandStrSize - 1] = 0;
		  headFile = open (startCommandStr, O_BINARY | O_RDWR);
		  fstat (headFile, &st);
		  headFileSize = st.st_size;
		  headFileMemMap =
		    mmap (0, headFileSize, PROT_READ, MAP_SHARED, headFile,
			  0);
		  madvise (headFileMemMap, headFileSize, MADV_WILLNEED);
		  startCommandStrSize--;
		  for (j = 0; j != headFileSize; j++)
		    {
		      if (headFileMemMap[j] == 'r'
			  && headFileMemMap[j + 1] == 'e'
			  && headFileMemMap[j + 2] == 'f'
			  && headFileMemMap[j + 3] == ':'
			  && headFileMemMap[j + 4] == ' ')
			{
			  j += 5;
			  headFileMemMap += j;
			  startCommandStrSize -= HEADLocStrSize;
			  for (k = 0;
			       headFileMemMap[k] != '\n' && k != headFileSize;
			       k++);
			  appendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &headFileMemMap, &k,
					  &startCommandStrSizeMax);
			  startCommandStrSize++;
			  setupDstAppendInternal (&startCommandStr,
						  &startCommandStrSize,
						  &startCommandStrSizeMax);
			  startCommandStr[startCommandStrSize - 1] = 0;
			  headFileMemMap -= j;
			  j = headFileSize - 1;
			}
		    }
		  munmap (headFileMemMap, headFileSize);
		  close (headFile);
		  headFile = open (startCommandStr, O_BINARY | O_RDWR);
		  fstat (headFile, &st);
		  headFileSize = st.st_size;
		  headFileMemMap =
		    mmap (0, headFileSize, PROT_READ, MAP_SHARED, headFile,
			  0);
		  madvise (headFileMemMap, headFileSize, MADV_WILLNEED);
		  for (j = 0; headFileMemMap[j] != '\n' && j != headFileSize;
		       j++);
		  startCommandStrSize =
		    installDirNameSize - 2 + varLocStrSize;
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &aGLocStr,
					  &aGLocStrSize,
					  &startCommandStrSizeMax);
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &statusLocStr,
					  &statusLocStrSize,
					  &startCommandStrSizeMax);
		  startCommandStrSize++;
		  setupDstAppendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &startCommandStrSizeMax);
		  startCommandStr[startCommandStrSize - 1] = 0;
		  statusFile =
		    open (startCommandStr,
			  O_BINARY | O_RDWR | O_CREAT | O_APPEND,
			  S_IRUSR | S_IWUSR);
		  fstat (statusFile, &st);
		  statusFileSize = st.st_size;
		  statusFileMemAlloc =
		    mmap (0, statusFileSize, PROT_READ | PROT_WRITE,
			  MAP_SHARED, statusFile, 0);
		  madvise (statusFileMemAlloc, statusFileSize, MADV_WILLNEED);
		  statusFileMemAllocPos = 0;
		  gotonewline = false;
		  creatednew = false;
		  for (k = 0; k != statusFileSize; k++)
		    {
		      if (gotonewline)
			{
			  if (creatednew)
			    {
			      if (k - statusFileMemAllocPos == argvSize[i])
				{
				  if (statusFileMemAlloc[k] != '\n')
				    {
				      creatednew = false;
				      k = statusFileSize - 1;
				    }
				}
			      else
				{
				  creatednew =
				    argv[i][k - statusFileMemAllocPos] ==
				    statusFileMemAlloc[k];
				}
			    }
			}
		      else
			{
			  if (creatednew
			      && k - statusFileMemAllocPos - 1 == argvSize[i])
			    {
			      k = statusFileSize - 1;
			    }
			  else
			    {
			      creatednew = false;
			      if (!strncmp
				  (statusFileMemAlloc + k, "Package: ", 9))
				{
				  k += 9;
				  statusFileMemAllocPos = k;
				  if (k - statusFileMemAllocPos ==
				      argvSize[i])
				    {
				      k = statusFileSize - 1;
				    }
				  else
				    {
				      creatednew =
					argv[i][k - statusFileMemAllocPos] ==
					statusFileMemAlloc[k];
				    }
				}
			    }
			}
		      gotonewline = statusFileMemAlloc[k] != '\n';
		    }
		  if (creatednew)
		    {
		      statusFileMemAllocPos += argvSize[i] + 1;
		      creatednew = false;
		      gotonewline = false;
		      for (k = statusFileMemAllocPos; k != statusFileSize;
			   k++)
			{
			  if (!gotonewline && statusFileMemAlloc[k] == '\n')
			    {
			      break;
			    }
			  gotonewline = statusFileMemAlloc[k] != '\n';
			  if (gotonewline)
			    {
			      if (creatednew)
				{
				  if (k - statusFileMemAllocPos == j)
				    {
				      k = statusFileSize - 1;
				    }
				  else
				    {
				      statusFileMemAlloc[k] =
					headFileMemMap[k -
						       statusFileMemAllocPos];
				    }
				}
			    }
			  else
			    {
			      if (creatednew
				  && k - statusFileMemAllocPos - 1 == j)
				{
				  k = statusFileSize - 1;
				}
			      else
				{
				  creatednew = false;
				  if (statusFileMemAlloc[k - 1] != '\n'
				      && !strncmp (statusFileMemAlloc + k,
						   "\nCommitHash: ", 13))
				    {
				      k += 13;
				      creatednew =
					k - statusFileMemAllocPos != j;
				      statusFileMemAllocPos = k;
				      if (k - statusFileMemAllocPos == j)
					{
					  k = statusFileSize - 1;
					}
				      else
					{
					  creatednew = true;
					  statusFileMemAlloc[k] =
					    headFileMemMap[k -
							   statusFileMemAllocPos];
					}
				    }
				}
			    }
			}
		      creatednew = true;
		    }
		  munmap (statusFileMemAlloc, statusFileSize);
		  if (!creatednew)
		    {
		      statusFileSize =
			1 + packageEntryStrSize + argvSize[i] + 1 +
			statusEntryStrSize + statusEntryInstalledStrSize + 1 +
			commitHashEntryStrSize + j + 1;
		      statusFileMemAlloc = (char *) malloc (statusFileSize);
		      statusFileMemAllocPos = 0;
		      statusFileMemAlloc[statusFileMemAllocPos] = '\n';
		      statusFileMemAllocPos++;
		      memcpy (statusFileMemAlloc + statusFileMemAllocPos,
			      packageEntryStr, packageEntryStrSize);
		      statusFileMemAllocPos += packageEntryStrSize;
		      memcpy (statusFileMemAlloc + statusFileMemAllocPos,
			      argv[i], argvSize[i]);
		      statusFileMemAllocPos += argvSize[i];
		      statusFileMemAlloc[statusFileMemAllocPos] = '\n';
		      statusFileMemAllocPos++;
		      memcpy (statusFileMemAlloc + statusFileMemAllocPos,
			      statusEntryStr, statusEntryStrSize);
		      statusFileMemAllocPos += statusEntryStrSize;
		      memcpy (statusFileMemAlloc + statusFileMemAllocPos,
			      statusEntryInstalledStr,
			      statusEntryInstalledStrSize);
		      statusFileMemAllocPos += statusEntryInstalledStrSize;
		      statusFileMemAlloc[statusFileMemAllocPos] = '\n';
		      statusFileMemAllocPos++;
		      memcpy (statusFileMemAlloc + statusFileMemAllocPos,
			      commitHashEntryStr, commitHashEntryStrSize);
		      statusFileMemAllocPos += commitHashEntryStrSize;
		      memcpy (statusFileMemAlloc + statusFileMemAllocPos,
			      headFileMemMap, j);
		      statusFileMemAllocPos += j;
		      statusFileMemAlloc[statusFileMemAllocPos] = '\n';
		      statusFileMemAllocPos++;
		      write (statusFile, statusFileMemAlloc,
			     statusFileMemAllocPos);
		      free (statusFileMemAlloc);
		    }
		  close (statusFile);
		  munmap (headFileMemMap, headFileSize);
		  close (headFile);
		}
	    }
	}
      if (!strcmp (argv[1], "remove") && argc != 2)
	{
	  printf ("Reading package lists... ");
	  packageListPathStrSize = installDirNameSize + infoLocStrSize;
	  packageListPathStr = malloc (packageListPathStrSize);
	  packageListPathStrSizeMax = packageListPathStrSize;
	  packageListPathStrSize = 0;
	  installDirNameSize--;
	  appendInternal (&packageListPathStr, &packageListPathStrSize,
			  &installDir, &installDirNameSize,
			  &packageListPathStrSizeMax);
	  installDirNameSize++;
	  appendInternalSrcConst (&packageListPathStr,
				  &packageListPathStrSize, &infoLocStr,
				  &infoLocStrSize,
				  &packageListPathStrSizeMax);
	  listDelFiles = (int *) malloc ((argc - 2) * sizeof (listDelFiles));
	  listDelFilesMemAlloc =
	    (char **) malloc ((argc - 2) * sizeof (listDelFilesMemAlloc));
	  listDelFilesSize =
	    (size_t *) malloc ((argc - 2) * sizeof (listDelFilesSize));
	  listDelFilesStartILFE =
	    (struct internalListFileEntry **) malloc ((argc - 2) *
						      sizeof
						      (listDelFilesStartILFE));
	  listDelFilesNumILFE =
	    (size_t *) malloc ((argc - 2) * sizeof (listDelFilesNumILFE));
	  for (i = 2; i != argc; i++)
	    {
	      appendInternal (&packageListPathStr, &packageListPathStrSize,
			      &argv[i], &argvSize[i],
			      &packageListPathStrSizeMax);
	      appendInternalSrcConst (&packageListPathStr,
				      &packageListPathStrSize,
				      &listFileExtensionStr,
				      &listFileExtensionStrSize,
				      &packageListPathStrSizeMax);
	      packageListPathStrSize++;
	      setupDstAppendInternal (&packageListPathStr,
				      &packageListPathStrSize,
				      &packageListPathStrSizeMax);
	      packageListPathStr[packageListPathStrSize - 1] = 0;
	      listDelFiles[i - 2] =
		open (packageListPathStr, O_BINARY | O_RDONLY);
	      fstat (listDelFiles[i - 2], &st);
	      listDelFilesSize[i - 2] = st.st_size;
	      listDelFilesMemAlloc[i - 2] =
		mmap (0, listDelFilesSize[i - 2], PROT_READ, MAP_SHARED,
		      listDelFiles[i - 2], 0);
	      if (listDelFilesMemAlloc[i - 2] == MAP_FAILED)
		{
		  chmod (packageListPathStr, S_IRUSR | S_IWUSR | S_IXUSR);
		  chmod (packageListPathStr,
			 S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |
			 S_IXGRP);
		  chmod (packageListPathStr,
			 S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |
			 S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
		  listDelFilesMemAlloc[i - 2] =
		    mmap (0, listDelFilesSize[i - 2], PROT_READ, MAP_SHARED,
			  listDelFiles[i - 2], 0);
		  chmod (packageListPathStr, st.st_mode);
		}
	      madvise (listDelFilesMemAlloc[i - 2], listDelFilesSize[i - 2],
		       MADV_WILLNEED);
	      packageListPathStrSize -=
		argvSize[i] + listFileExtensionStrSize + 1;
	    }
	  printf ("Done\n");
	  printf ("Building dependency tree\n");
	  for (i = 2; i != argc; i++)
	    {
	      madvise (listDelFilesMemAlloc[i - 2], listDelFilesSize[i - 2],
		       MADV_WILLNEED);
	      listDelFileTruthAlloc =
		(char *) malloc ((listDelFilesSize[i - 2] / 8) +
				 (listDelFilesSize[i - 2] % 8 != 0));
	      listDelFileMemAllocPos = 0;
	      numILFE = 0;
	      for (j = 0; j != (listDelFilesSize[i - 2] / 8); j++)
		{
		  listDelFileTruthAlloc[j] =
		    (listDelFilesMemAlloc[i - 2][listDelFileMemAllocPos] ==
		     '\n');
		  listDelFileTruthAlloc[j] +=
		    (listDelFilesMemAlloc[i - 2][listDelFileMemAllocPos + 1]
		     == '\n') << 1;
		  listDelFileTruthAlloc[j] +=
		    (listDelFilesMemAlloc[i - 2][listDelFileMemAllocPos + 2]
		     == '\n') << 2;
		  listDelFileTruthAlloc[j] +=
		    (listDelFilesMemAlloc[i - 2][listDelFileMemAllocPos + 3]
		     == '\n') << 3;
		  listDelFileTruthAlloc[j] +=
		    (listDelFilesMemAlloc[i - 2][listDelFileMemAllocPos + 4]
		     == '\n') << 4;
		  listDelFileTruthAlloc[j] +=
		    (listDelFilesMemAlloc[i - 2][listDelFileMemAllocPos + 5]
		     == '\n') << 5;
		  listDelFileTruthAlloc[j] +=
		    (listDelFilesMemAlloc[i - 2][listDelFileMemAllocPos + 6]
		     == '\n') << 6;
		  listDelFileTruthAlloc[j] +=
		    (listDelFilesMemAlloc[i - 2][listDelFileMemAllocPos + 7]
		     == '\n') << 7;
		  listDelFileMemAllocPos += 8;
		  numILFE +=
		    (listDelFileTruthAlloc[j] & 1) +
		    ((listDelFileTruthAlloc[j] >> 1) & 1) +
		    ((listDelFileTruthAlloc[j] >> 2) & 1) +
		    ((listDelFileTruthAlloc[j] >> 3) & 1) +
		    ((listDelFileTruthAlloc[j] >> 4) & 1) +
		    ((listDelFileTruthAlloc[j] >> 5) & 1) +
		    ((listDelFileTruthAlloc[j] >> 6) & 1) +
		    ((listDelFileTruthAlloc[j] >> 7) & 1);
		}
	      if (listDelFilesSize[i - 2] % 8 != 0)
		{
		  listDelFileTruthAlloc[j] = 0;
		}
	      for (listDelFileMemAllocPos = 0;
		   listDelFileMemAllocPos != (listDelFilesSize[i - 2] % 8);
		   listDelFileMemAllocPos++)
		{
		  listDelFileTruthAlloc[j] +=
		    (listDelFilesMemAlloc[i - 2]
		     [j * 8 + listDelFileMemAllocPos] ==
		     '\n') << listDelFileMemAllocPos;
		  numILFE +=
		    (listDelFileTruthAlloc[j] >> listDelFileMemAllocPos) & 1;
		}
	      if (numILFE)
		{
		  numILFE++;
		  startILFE =
		    (struct internalListFileEntry *) malloc (numILFE *
							     sizeof
							     (*startILFE));
		  memset (startILFE, 0, numILFE * sizeof (*startILFE));
		  numILFE = 0;
		  for (j = 0; j != listDelFilesSize[i - 2]; j++)
		    {
		      register char abitlogic =
			((listDelFileTruthAlloc[j / 8]) >> (j % 8)) & 1;
		      startILFE[numILFE].fileLocSize += 1 - abitlogic;
		      numILFE += abitlogic;
		    }
		  numILFE++;
		  currCharPtr = listDelFilesMemAlloc[i - 2];
		  for (j = 0; j != numILFE; j++)
		    {
		      startILFE[j].fileLoc = currCharPtr;
		      currCharPtr += startILFE[j].fileLocSize;
		      currCharPtr++;
		      startILFE[j].isdone = 0;
		    }
		  listDelFilesStartILFE[i - 2] = startILFE;
		}
	      listDelFilesNumILFE[i - 2] = numILFE;
	    }
	  printf ("Reading state information... ");
	  listDelFilesDataSize = 0;
	  for (i = 2; i != argc; i++)
	    {
	      numILFE = listDelFilesNumILFE[i - 2];
	      if (numILFE)
		{
		  startILFE = listDelFilesStartILFE[i - 2];
		  for (j = 0; j != numILFE; j++)
		    {
		      startILFE[j].isdone = startILFE[j].fileLocSize == 0;
		      if (!startILFE[j].isdone)
			{
			  packageListPathStrSize = installDirNameSize;
			  appendInternal (&packageListPathStr,
					  &packageListPathStrSize,
					  &startILFE[j].fileLoc,
					  &startILFE[j].fileLocSize,
					  &packageListPathStrSizeMax);
			  packageListPathStrSize++;
			  setupDstAppendInternal (&packageListPathStr,
						  &packageListPathStrSize,
						  &packageListPathStrSizeMax);
			  packageListPathStr[packageListPathStrSize - 1] = 0;
			  stat (packageListPathStr, &st);
			  if (!st.st_size)
			    {
			      fd =
				open (packageListPathStr,
				      O_BINARY | O_RDONLY);
			      fstat (fd, &st);
			      close (fd);
			    }
			  listDelFilesDataSize += st.st_size;
			}
		    }
		}
	    }
	  printf ("Done\n");
	  printf ("The following package will be REMOVED:\n");
	  printf (" ");
	  for (i = 2; i != argc; i++)
	    {
	      printf (" %s", argv[i]);
	    }
	  printf ("\n");
	  printf
	    ("0 upgraded, 0 newly installed, %d to remove and 0 not upgraded.\n",
	     argc - 2);
	  printf ("After this operation, ");
	  if (listDelFilesDataSize == 0)
	    {
	      printf ("0 kB");
	    }
	  else if (listDelFilesDataSize < 1024)
	    {
	      printf ("%lld bytes", listDelFilesDataSize);
	    }
	  else if (listDelFilesDataSize < 1048576)
	    {
	      printf ("%lld kB", listDelFilesDataSize / 1024);
	    }
	  else if (listDelFilesDataSize < 1073741824)
	    {
	      printf ("%lld mB", listDelFilesDataSize / 1048576);
	    }
	  else if (listDelFilesDataSize < 1099511627776)
	    {
	      printf ("%lld gB", listDelFilesDataSize / 1073741824);
	    }
	  else
	    {
	      printf ("%lld tB", listDelFilesDataSize / 1099511627776);
	    }
	  printf (" disk space will be freed.\n");
	  printf ("Do you want to continue? [Y/n] ");
	  if (getchar () != 'n')
	    {
	      for (i = 2; i != argc; i++)
		{
		  numILFE = 0;
		  if (listDelFilesNumILFE[i - 2])
		    {
		      startILFE = listDelFilesStartILFE[i - 2];
		      for (j = 0; j != listDelFilesNumILFE[i - 2]; j++)
			{
			  if (!startILFE[j].isdone)
			    {
			      numILFE++;
			    }
			}
		    }
		  printf
		    ("(Reading database ... %lld files and directories currently installed.)\n",
		     numILFE);
		  printf ("Removing %s ...\n", argv[i]);
		  numILFE = listDelFilesNumILFE[i - 2];
		  if (numILFE)
		    {
		      startILFE = listDelFilesStartILFE[i - 2];
		      for (j = 0; j != numILFE; j++)
			{
			  if (!startILFE[j].isdone)
			    {
			      packageListPathStrSize = installDirNameSize;
			      appendInternal (&packageListPathStr,
					      &packageListPathStrSize,
					      &startILFE[j].fileLoc,
					      &startILFE[j].fileLocSize,
					      &packageListPathStrSizeMax);
			      packageListPathStrSize++;
			      setupDstAppendInternal (&packageListPathStr,
						      &packageListPathStrSize,
						      &packageListPathStrSizeMax);
			      packageListPathStr[packageListPathStrSize - 1] =
				0;
			      dirp = opendir (packageListPathStr);
			      if (dirp)
				{
				  closedir (dirp);
				}
			      else
				{
				  chmod (packageListPathStr, S_IWUSR);
				  startILFE[j].isdone =
				    1 - (remove (packageListPathStr) & 1);
				}
			    }
			}
		      for (j = 0; j != numILFE; j++)
			{
			  if (!startILFE[j].isdone)
			    {
			      packageListPathStrSize = installDirNameSize;
			      appendInternal (&packageListPathStr,
					      &packageListPathStrSize,
					      &startILFE[j].fileLoc,
					      &startILFE[j].fileLocSize,
					      &packageListPathStrSizeMax);
			      packageListPathStrSize++;
			      setupDstAppendInternal (&packageListPathStr,
						      &packageListPathStrSize,
						      &packageListPathStrSizeMax);
			      packageListPathStr[packageListPathStrSize - 1] =
				0;
			      startILFE[j].isdone =
				1 - (rmdir (packageListPathStr) & 1);
			    }
			}
		      for (j = 0; j != numILFE; j++)
			{
			  if (!startILFE[numILFE - j - 1].isdone)
			    {
			      packageListPathStrSize = installDirNameSize;
			      appendInternal (&packageListPathStr,
					      &packageListPathStrSize,
					      &startILFE[numILFE - j -
							 1].fileLoc,
					      &startILFE[numILFE - j -
							 1].fileLocSize,
					      &packageListPathStrSizeMax);
			      packageListPathStrSize++;
			      setupDstAppendInternal (&packageListPathStr,
						      &packageListPathStrSize,
						      &packageListPathStrSizeMax);
			      packageListPathStr[packageListPathStrSize - 1] =
				0;
			      startILFE[numILFE - j - 1].isdone =
				1 - (rmdir (packageListPathStr) & 1);
			    }
			}
		      for (j = 0; j != numILFE; j++)
			{
			  if (!startILFE[j].isdone)
			    {
			      packageListPathStrSize = installDirNameSize;
			      appendInternal (&packageListPathStr,
					      &packageListPathStrSize,
					      &startILFE[j].fileLoc,
					      &startILFE[j].fileLocSize,
					      &packageListPathStrSizeMax);
			      packageListPathStrSize++;
			      setupDstAppendInternal (&packageListPathStr,
						      &packageListPathStrSize,
						      &packageListPathStrSizeMax);
			      packageListPathStr[packageListPathStrSize - 1] =
				0;
			      dirp = opendir (packageListPathStr);
			      if (dirp)
				{
				  closedir (dirp);
				}
			      else
				{
				  chmod (packageListPathStr, S_IWUSR);
				  startILFE[j].isdone =
				    1 - (remove (packageListPathStr) & 1);
				}
			    }
			}
		      for (j = 0; j != numILFE; j++)
			{
			  if (!startILFE[j].isdone)
			    {
			      packageListPathStrSize = installDirNameSize;
			      appendInternal (&packageListPathStr,
					      &packageListPathStrSize,
					      &startILFE[j].fileLoc,
					      &startILFE[j].fileLocSize,
					      &packageListPathStrSizeMax);
			      packageListPathStrSize++;
			      setupDstAppendInternal (&packageListPathStr,
						      &packageListPathStrSize,
						      &packageListPathStrSizeMax);
			      packageListPathStr[packageListPathStrSize - 1] =
				0;
			      startILFE[j].isdone =
				1 - (rmdir (packageListPathStr) & 1);
			    }
			}
		      for (j = 0; j != numILFE; j++)
			{
			  if (!startILFE[numILFE - j - 1].isdone)
			    {
			      packageListPathStrSize = installDirNameSize;
			      appendInternal (&packageListPathStr,
					      &packageListPathStrSize,
					      &startILFE[numILFE - j -
							 1].fileLoc,
					      &startILFE[numILFE - j -
							 1].fileLocSize,
					      &packageListPathStrSizeMax);
			      packageListPathStrSize++;
			      setupDstAppendInternal (&packageListPathStr,
						      &packageListPathStrSize,
						      &packageListPathStrSizeMax);
			      packageListPathStr[packageListPathStrSize - 1] =
				0;
			      startILFE[numILFE - j - 1].isdone =
				1 - (rmdir (packageListPathStr) & 1);
			    }
			}
		    }
		  close (listDelFiles[i - 2]);
		  munmap (listDelFilesMemAlloc[i - 2],
			  listDelFilesSize[i - 2]);
		  packageListPathStrSize = installDirNameSize;
		  appendInternalSrcConst (&packageListPathStr,
					  &packageListPathStrSize,
					  &infoLocStr, &infoLocStrSize,
					  &packageListPathStrSizeMax);
		  appendInternal (&packageListPathStr,
				  &packageListPathStrSize, &argv[i],
				  &argvSize[i], &packageListPathStrSizeMax);
		  appendInternalSrcConst (&packageListPathStr,
					  &packageListPathStrSize,
					  &listFileExtensionStr,
					  &listFileExtensionStrSize,
					  &packageListPathStrSizeMax);
		  packageListPathStrSize++;
		  setupDstAppendInternal (&packageListPathStr,
					  &packageListPathStrSize,
					  &packageListPathStrSizeMax);
		  packageListPathStr[packageListPathStrSize - 1] = 0;
		  if (remove (packageListPathStr))
		    {
		      chmod (packageListPathStr, S_IWUSR);
		      remove (packageListPathStr);
		    }
		}
	      packageListPathStrSize = installDirNameSize;
	      appendInternalSrcConst (&packageListPathStr,
				      &packageListPathStrSize, &varLocStr,
				      &varLocStrSize,
				      &packageListPathStrSizeMax);
	      appendInternalSrcConst (&packageListPathStr,
				      &packageListPathStrSize, &aGLocStr,
				      &aGLocStrSize,
				      &packageListPathStrSizeMax);
	      appendInternalSrcConst (&packageListPathStr,
				      &packageListPathStrSize, &statusLocStr,
				      &statusLocStrSize,
				      &packageListPathStrSizeMax);
	      packageListPathStrSize++;
	      setupDstAppendInternal (&packageListPathStr,
				      &packageListPathStrSize,
				      &packageListPathStrSizeMax);
	      packageListPathStr[packageListPathStrSize - 1] = 0;
	      statusFile =
		open (packageListPathStr, O_BINARY | O_RDWR | O_APPEND);
	      statusFileSize = 0;
	      if (statusFile != -1)
		{
		  fstat (statusFile, &st);
		  statusFileSize = st.st_size;
		  if (statusFileSize != 0 && statusFileSize != 1
		      && statusFileSize != 2 && statusFileSize != 3
		      && statusFileSize != 4 && statusFileSize != 5
		      && statusFileSize != 6 && statusFileSize != 7
		      && statusFileSize != 8)
		    {
		      statusFileMemAlloc =
			mmap (0, statusFileSize, PROT_READ | PROT_WRITE,
			      MAP_SHARED, statusFile, 0);
		      madvise (statusFileMemAlloc, statusFileSize,
			       MADV_WILLNEED);
		    }
		  else
		    {
		      statusFileSize = 0;
		    }
		}
	      if (statusFileSize)
		{
		  selectedLength1 = 0;
		  statusFileRangesPos = 0;
		  statusFileRangesSize = 0;
		  statusFileRangesPosMax = 0;
		  statusFileRangesSizeMax = 0;
		  *statusFileRanges = (size_t *) malloc (sizeof (size_t) * 2);
		  newstatusFileSize = statusFileSize;
		  for (i = 2; i != argc; i++)
		    {
		      gotonewline = false;
		      statusFileMemAllocPos = 0;
		      for (j = 0; j != statusFileSize - 9; j++)
			{
			  if (statusFileMemAllocPos)
			    {
			      if (j - statusFileMemAllocPos == argvSize[i])
				{
				  if (statusFileMemAlloc[j] == '\n')
				    {
				      j = statusFileSize - 10;
				    }
				  else
				    {
				      statusFileMemAllocPos = 0;
				    }
				}
			      else
				{
				  if (statusFileMemAlloc[j] !=
				      argv[i][j - statusFileMemAllocPos])
				    {
				      statusFileMemAllocPos = 0;
				    }
				}
			    }
			  if (!gotonewline)
			    {
			      if (!strncmp
				  (statusFileMemAlloc + j, "Package: ", 9))
				{
				  j += 9;
				  statusFileMemAllocPos = j;
				}
			    }
			  gotonewline = statusFileMemAlloc[j] != '\n';
			}
		      if (!statusFileMemAllocPos)
			{
			  statusFileMemAllocPos = statusFileSize + 9;
			}
		      statusFileRanges[statusFileRangesPos]
			[statusFileRangesSize] = statusFileMemAllocPos - 9;
		      if (statusFileMemAllocPos == statusFileSize + 9)
			{
			  statusFileMemAllocPos -= 18;
			}
		      gotonewline = false;
		      j = statusFileMemAllocPos;
		      for (statusFileMemAllocPos = 0; j != statusFileSize - 9;
			   j++)
			{
			  if (!gotonewline)
			    {
			      if (!strncmp
				  (statusFileMemAlloc + j, "Package: ", 9))
				{
				  statusFileMemAllocPos = j;
				  j = statusFileSize - 10;
				}
			    }
			  gotonewline = statusFileMemAlloc[j] != '\n';
			}
		      if (!statusFileMemAllocPos)
			{
			  statusFileMemAllocPos = statusFileSize;
			}
		      statusFileRanges[statusFileRangesPos]
			[statusFileRangesSize + 1] = statusFileMemAllocPos;
		      statusFileRangesSize += 2;
		      selectedLength1++;
		      if (statusFileRangesSize ==
			  1 << (statusFileRangesPos + 1))
			{
			  statusFileRangesSize = 0;
			  statusFileRangesPos++;
			  statusFileRanges[statusFileRangesPos] =
			    (size_t *) malloc (sizeof (size_t) *
					       2 << statusFileRangesPos);
			}
		    }
		  statusFileRanges[statusFileRangesPos][statusFileRangesSize]
		    = statusFileSize;
		  statusFileRanges[statusFileRangesPos][statusFileRangesSize +
							1] = statusFileSize;
		  madvise (statusFileMemAlloc, statusFileSize, MADV_WILLNEED);
		  statusFileRangesPosMax = statusFileRangesPos;
		  statusFileRangesSizeMax = statusFileRangesSize;
		  radixes1 = (size_t *) malloc (sizeof (size_t) * 256 * 8);
		  radixoffPos1 = (size_t *) malloc (sizeof (size_t) * 256);
		  radixoffSize1 = (size_t *) malloc (sizeof (size_t) * 256);
		  radix1 = radixes1;
		  statusFileRangesPos = statusFileRangesSize = 0;
		  i = 0;
		  j = 0;
		  k = 0;
		  if (!selectedLength1)
		    {
		      k = sizeof (i);
		    }
		  while (k != sizeof (i))
		    {
		      for (m = 0; m != 256; m++)
			{
			  radix1[m] = 0;
			}
		      s1 = 64 - 8 - (k * 8);
		      lPos1 = statusFileRangesPos;
		      lSize1 = statusFileRangesSize;
		      for (m = selectedLength1; m; m--)
			{
			  radix1[(statusFileRanges[lPos1][lSize1] >> s1) &
				 0xff]++;
			  lSize1 += 2;
			  if (lSize1 == 1 << (lPos1 + 1))
			    {
			      lSize1 = 0;
			      lPos1++;
			    }
			}
		      lPos1 = statusFileRangesPos;
		      lSize1 = statusFileRangesSize;
		      for (m = 0; m != 256; m++)
			{
			  radixoffPos1[m] = lPos1;
			  radixoffSize1[m] = lSize1;
			  n = radix1[m] * 2;
			  lSize1 += n;
			  if (lSize1 <= 1 << (lPos1 + 1))
			    {
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			  else
			    {
			      lSize1 = (1 << (lPos1 + 1)) - lSize1 + n;
			      n -= lSize1;
			      while (n != 0)
				{
				  lPos1++;
				  if (n > 1 << (lPos1 + 1))
				    {
				    }
				  else
				    {
				      lSize1 = n - (1 << (lPos1 + 1));
				      n = 1 << (lPos1 + 1);
				    }
				  lSize1 += 1 << (lPos1 + 1);
				  n -= 1 << (lPos1 + 1);
				}
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			}
		      lPos1 = statusFileRangesPos;
		      lSize1 = statusFileRangesSize;
		      for (m = 0; m != 256; m++)
			{
			  n = radix1[m] * 2;
			  lSize1 += n;
			  if (lSize1 <= 1 << (lPos1 + 1))
			    {
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			  else
			    {
			      lSize1 = (1 << (lPos1 + 1)) - lSize1 + n;
			      n -= lSize1;
			      while (n != 0)
				{
				  lPos1++;
				  if (n > 1 << (lPos1 + 1))
				    {
				    }
				  else
				    {
				      lSize1 = n - (1 << (lPos1 + 1));
				      n = 1 << (lPos1 + 1);
				    }
				  lSize1 += 1 << (lPos1 + 1);
				  n -= 1 << (lPos1 + 1);
				}
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			  while (radixoffPos1[m] != lPos1
				 || radixoffSize1[m] != lSize1)
			    {
			      n =
				statusFileRanges[radixoffPos1[m]]
				[radixoffSize1[m]];
			      o =
				statusFileRanges[radixoffPos1[m]]
				[radixoffSize1[m] + 1];
			      statusFileRanges[radixoffPos1[m]][radixoffSize1
								[m]] =
				statusFileRanges[radixoffPos1
						 [(n >> s1) &
						  0xff]][radixoffSize1[(n >>
									s1) &
								       0xff]];
			      statusFileRanges[radixoffPos1[m]][radixoffSize1
								[m] + 1] =
				statusFileRanges[radixoffPos1
						 [(n >> s1) &
						  0xff]][radixoffSize1[(n >>
									s1) &
								       0xff] +
							 1];
			      statusFileRanges[radixoffPos1[(n >> s1) & 0xff]]
				[radixoffSize1[(n >> s1) & 0xff]] = n;
			      statusFileRanges[radixoffPos1[(n >> s1) & 0xff]]
				[radixoffSize1[(n >> s1) & 0xff] + 1] = o;
			      n = (n >> s1) & 0xff;
			      radixoffSize1[n] += 2;
			      if (radixoffSize1[n] ==
				  1 << (radixoffPos1[n] + 1))
				{
				  radixoffSize1[n] = 0;
				  radixoffPos1[n]++;
				}
			    }
			}
		      selectedLength1 = radix1[(i >> s1) & 0xff];
		      if (k == sizeof (i) - 1
			  && (lPos1 != statusFileRangesPosMax
			      || lSize1 != statusFileRangesSizeMax))
			{
			  i |= 0xff;
			  statusFileRangesPos = lPos1;
			  statusFileRangesSize = lSize1;
			  while (s1 != sizeof (i) * 8
				 && ((i >> s1) & 0xff) == 0xff)
			    {
			      s1 += 8;
			      i >>= s1;
			      i++;
			      i <<= s1;
			      radix1 -= 256;
			      k--;
			    }
			  selectedLength1 = radix1[(i >> s1) & 0xff];
			}
		      if (!selectedLength1)
			{
			  while (!selectedLength1 && s1 != sizeof (i) * 8)
			    {
			      i >>= s1;
			      for (m = i & 0xff; !selectedLength1 && m != 256;
				   m++)
				{
				  selectedLength1 = radix1[m];
				}
			      i >>= 8;
			      i <<= 8;
			      i += m - 1;
			      i <<= s1;
			      if (m == 256)
				{
				  s1 += 8;
				  i >>= s1;
				  i++;
				  i <<= s1;
				  radix1 -= 256;
				  k--;
				  while (s1 != sizeof (i) * 8
					 && ((i >> s1) & 0xff) == 0xff)
				    {
				      s1 += 8;
				      i >>= s1;
				      i++;
				      i <<= s1;
				      radix1 -= 256;
				      k--;
				    }
				  if (s1 == sizeof (i) * 8)
				    {
				      k = sizeof (i);
				    }
				  else
				    {
				      selectedLength1 =
					radix1[(i >> s1) & 0xff];
				    }
				}
			    }
			}
		      radix1 += 256;
		      k++;
		    }
		  statusFileRangesPos = statusFileRangesPosMax;
		  statusFileRangesSize = statusFileRangesSizeMax;
		  while ((statusFileRangesPos || statusFileRangesSize)
			 &&
			 statusFileRanges[statusFileRangesPos]
			 [statusFileRangesSize + 1] == statusFileSize)
		    {
		      if (!statusFileRangesSize)
			{
			  statusFileRangesPos--;
			  statusFileRangesSize =
			    1 << (statusFileRangesPos + 1);
			}
		      statusFileRangesSize -= 2;
		    }
		  k =
		    statusFileRanges[statusFileRangesPos]
		    [statusFileRangesSize] != statusFileSize
		    &&
		    statusFileRanges[statusFileRangesPos][statusFileRangesSize
							  + 1] ==
		    statusFileSize;
		  if (!k
		      && (statusFileRangesPos != statusFileRangesPosMax
			  || statusFileRangesSize != statusFileRangesSizeMax))
		    {
		      statusFileRangesSize += 2;
		      if (statusFileRangesSize ==
			  1 << (statusFileRangesPos + 1))
			{
			  statusFileRangesSize = 0;
			  statusFileRangesPos++;
			}
		    }
		  k |=
		    statusFileRanges[statusFileRangesPos]
		    [statusFileRangesSize] != statusFileSize
		    &&
		    statusFileRanges[statusFileRangesPos][statusFileRangesSize
							  + 1] ==
		    statusFileSize;
		  statusFileRanges[statusFileRangesPos][statusFileRangesSize +
							1] -= k;
		  statusFileRangesPos = statusFileRangesSize = 0;
		  for (j = i = 0; j != statusFileSize; i++)
		    {
		      while (j ==
			     statusFileRanges[statusFileRangesPos]
			     [statusFileRangesSize]
			     || j ==
			     statusFileRanges[statusFileRangesPos]
			     [statusFileRangesSize + 1])
			{
			  j =
			    statusFileRanges[statusFileRangesPos]
			    [statusFileRangesSize + 1];
			  statusFileRangesSize += 2;
			  if (statusFileRangesSize ==
			      1 << (statusFileRangesPos + 1))
			    {
			      statusFileRangesSize = 0;
			      statusFileRangesPos++;
			    }
			}
		      statusFileMemAlloc[i] = statusFileMemAlloc[j];
		      j++;
		    }
		  newstatusFileSize = i - k;
		  if (newstatusFileSize && newstatusFileSize != 1
		      && newstatusFileSize != 2
		      && statusFileMemAlloc[newstatusFileSize - 1] == '\n'
		      && statusFileMemAlloc[newstatusFileSize - 2] == '\n')
		    {
		      for (i = newstatusFileSize - 1;
			   i && statusFileMemAlloc[i] == '\n'; i--);
		      for (; i && statusFileMemAlloc[i] != '\n'; i--);
		      for (;
			   i != newstatusFileSize - 1
			   && statusFileMemAlloc[i] == '\n'; i++);
		      if (newstatusFileSize - i != 0
			  && newstatusFileSize - i != 1
			  && newstatusFileSize - i != 2
			  && newstatusFileSize - i != 3
			  && newstatusFileSize - i != 4
			  && newstatusFileSize - i != 5
			  && newstatusFileSize - i != 6
			  && newstatusFileSize - i != 7
			  && newstatusFileSize - i != 8
			  && strncmp (statusFileMemAlloc + i, "Package: ", 9))
			{
			  newstatusFileSize--;
			}
		    }
		  if (newstatusFileSize < statusFileSize)
		    {
		      munmap (statusFileMemAlloc, statusFileSize);
		      statusFileSize = newstatusFileSize;
		      ftruncate (statusFile, statusFileSize);
		      statusFileMemAlloc =
			mmap (0, statusFileSize, PROT_READ | PROT_WRITE,
			      MAP_SHARED, statusFile, 0);
		    }
		}
	    }
	}
      if (!strcmp (argv[1], "search") && argc != 2)
	{
	  printf ("Sorting... ");
	  printf ("Done\n");
	  printf ("Full Text Search... ");
	  printf ("Done\n");
	}
      if (!strcmp (argv[1], "update"))
	{
	  loadSourceList (&packageFileName, &packageFileNameSize, &installDir,
			  &installDirNameSize, sourceslistStr,
			  sourceslistStrSize, &packageFile, &packageFileSize,
			  &packageFileMemMap, &st, &numberOfPackageLists,
			  &gotonewline, &i, &packageList,
			  &packageListNameSizes, &packageListNameSize,
			  &execute, envp);
	  startCommandStrSize = 1;
	  startCommandStrSizeMax = 1;
	  startCommandStr = (char *) malloc (startCommandStrSize);
	  startCommandStrSize = 0;
	  installDirNameSize--;
	  appendInternal (&startCommandStr, &startCommandStrSize, &installDir,
			  &installDirNameSize, &startCommandStrSizeMax);
	  installDirNameSize++;
	  appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				  &varLocStr, &varLocStrSize,
				  &startCommandStrSizeMax);
	  appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				  &aGLocStr, &aGLocStrSize,
				  &startCommandStrSizeMax);
	  appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				  &statusLocStr, &statusLocStrSize,
				  &startCommandStrSizeMax);
	  startCommandStrSize++;
	  setupDstAppendInternal (&startCommandStr, &startCommandStrSize,
				  &startCommandStrSizeMax);
	  startCommandStr[startCommandStrSize - 1] = 0;
	  statusFile =
	    open (startCommandStr,
		  O_BINARY | O_RDONLY | O_CREAT | O_APPEND,
		  S_IRUSR | S_IWUSR);
	  fstat (statusFile, &st);
	  statusFileSize = st.st_size;
	  statusFileMemAlloc =
	    mmap (0, statusFileSize, PROT_READ, MAP_SHARED, statusFile, 0);
	  madvise (statusFileMemAlloc, statusFileSize, MADV_WILLNEED);
	  statusFileMemAllocPos = 0;
	  gotonewline = false;
	  firstiLIPE =
	    (struct internalListInstalledPackageEntry *)
	    malloc (sizeof (*firstiLIPE));
	  lastiLIPE = firstiLIPE;
	  lastiLIPE->packageNameStrSize = 0;
	  lastiLIPE->nextiLIPE = 0;
	  creatednew = false;
	  packageNameStrsNum = 0;
	  for (j = 0; j != statusFileSize; j++)
	    {
	      if (gotonewline)
		{
		  lastiLIPE->packageNameStrSize += creatednew;
		}
	      else
		{
		  creatednew = false;
		  if (!strncmp (statusFileMemAlloc + j, "Package: ", 9))
		    {
		      j += 9;
		      creatednew = true;
		      if (lastiLIPE->nextiLIPE)
			{
			  lastiLIPE = lastiLIPE->nextiLIPE;
			}
		      lastiLIPE->packageNameStr = statusFileMemAlloc + j;
		      lastiLIPE->packageNameStrSize = 0;
		      packageNameStrsNum++;
		      lastiLIPE->nextiLIPE =
			(struct internalListInstalledPackageEntry *)
			malloc (sizeof (*lastiLIPE));
		    }
		}
	      gotonewline = statusFileMemAlloc[j] != '\n';
	    }
	  free (lastiLIPE->nextiLIPE);
	  lastiLIPE->nextiLIPE = 0;
	  packageNameStrs =
	    malloc (packageNameStrsNum * sizeof (*packageNameStrs));
	  packageNameStrSizes =
	    malloc (packageNameStrsNum * sizeof (*packageNameStrSizes));
	  lastiLIPE = firstiLIPE;
	  i = 0;
	  while (lastiLIPE)
	    {
	      packageNameStrs[i] = lastiLIPE->packageNameStr;
	      packageNameStrSizes[i] = lastiLIPE->packageNameStrSize;
	      i++;
	      lastiLIPE = lastiLIPE->nextiLIPE;
	      free (firstiLIPE);
	      firstiLIPE = lastiLIPE;
	    }
	  if (firstiLIPE)
	    {
	      free (firstiLIPE);
	    }
	  startCommandStrSize--;
	  startCommandStrSize -= statusLocStrSize;
	  startCommandStrSize -= aGLocStrSize;
	  startCommandStrSize -= varLocStrSize;
	  appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				  &archivesLoc, &archivesLocSize,
				  &startCommandStrSizeMax);
	  packageNameStrListi =
	    malloc (packageNameStrsNum * sizeof (*packageNameStrListi));
	  for (i = 0; i != packageNameStrsNum; i++)
	    {
	      packageNameStrListi[i] = 0;
	    }
	  packageListUsed =
	    malloc (numberOfPackageLists * sizeof (*packageListUsed));
	  for (i = 0; i != numberOfPackageLists; i++)
	    {
	      packageListUsed[i] = 0;
	    }
	  for (i = 0; i != packageNameStrsNum; i++)
	    {
	      appendInternal (&startCommandStr, &startCommandStrSize,
			      packageNameStrs + i, packageNameStrSizes + i,
			      &startCommandStrSizeMax);
	      appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				      &gitLocStr, &gitLocStrSize,
				      &startCommandStrSizeMax);
	      appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				      &configLocStr, &configLocStrSize,
				      &startCommandStrSizeMax);
	      startCommandStrSize++;
	      setupDstAppendInternal (&startCommandStr, &startCommandStrSize,
				      &startCommandStrSizeMax);
	      startCommandStr[startCommandStrSize - 1] = 0;
	      configFile =
		open (startCommandStr,
		      O_BINARY | O_RDONLY | O_CREAT | O_APPEND,
		      S_IRUSR | S_IWUSR);
	      fstat (configFile, &st);
	      configFileSize = st.st_size;
	      configFileMemMap =
		mmap (0, configFileSize, PROT_READ, MAP_SHARED, configFile,
		      0);
	      madvise (configFileMemMap, configFileSize, MADV_WILLNEED);
	      configFileMemMapPos = 0;
	      for (j = 0; j != configFileSize; j++)
		{
		  if (configFileMemMap[j] == 'u'
		      && configFileMemMap[j + 1] == 'r'
		      && configFileMemMap[j + 2] == 'l'
		      && configFileMemMap[j + 3] == ' '
		      && configFileMemMap[j + 4] == '='
		      && configFileMemMap[j + 5] == ' ')
		    {
		      configFileMemMapPos = j + 6;
		      j = configFileSize - 1;
		    }
		}
	      for (j = 0; j != numberOfPackageLists; j++)
		{
		  if (configFileMemMapPos + packageListNameSizes[j] <=
		      configFileSize)
		    {
		      if (!strncmp
			  (configFileMemMap + configFileMemMapPos,
			   packageList[j], packageListNameSizes[j] - 1))
			{
			  packageNameStrListi[i] = j;
			  packageListUsed[j] = 1;
			}
		    }
		}
	      startCommandStrSize -= packageNameStrSizes[i];
	      startCommandStrSize -= gitLocStrSize;
	      startCommandStrSize -= configLocStrSize;
	      startCommandStrSize--;
	      close (configFile);
	      munmap (configFileMemMap, configFileSize);
	    }
	  *sargv = 0;
	  sargv[1] = "-C";
	  sargv[3] = "fetch";
	  sargv[4] = 0;
	  for (i = 0; i != numberOfPackageLists; i++)
	    {
	      if (packageListUsed[i])
		{
		  printf ("Get:1 %s main\n", packageList[i]);
		  for (j = 0; j != packageNameStrsNum; j++)
		    {
		      if (packageNameStrListi[j] == i)
			{
			  appendInternal (&startCommandStr,
					  &startCommandStrSize,
					  packageNameStrs + j,
					  packageNameStrSizes + j,
					  &startCommandStrSizeMax);
			  startCommandStrSize++;
			  setupDstAppendInternal (&startCommandStr,
						  &startCommandStrSize,
						  &startCommandStrSizeMax);
			  startCommandStr[startCommandStrSize - 1] = 0;
			  sargv[2] = startCommandStr;
			  if (*sargv)
			    {
			      waitid (P_PID, spid, &ssiginfo, WEXITED);
			    }
			  if (sargv[2])
			    {
			      *sargv = "git";
			    }
			  if (*sargv
			      && posix_spawnp (&spid, *sargv, NULL, NULL,
					       sargv, envp) != 0)
			    {
			      *sargv = 0;
			    }
			  startCommandStrSize -= packageNameStrSizes[j];
			  startCommandStrSize--;
			}
		    }
		}
	      else
		{
		  printf ("Hit:1 %s main\n", packageList[i]);
		}
	    }
	  if (*sargv)
	    {
	      waitid (P_PID, spid, &ssiginfo, WEXITED);
	    }
	}
      if ((!strcmp (argv[1], "upgrade") || !strcmp (argv[1], "dist-upgrade")))
	{
	  printf ("Reading package lists... ");
	  loadSourceList (&packageFileName, &packageFileNameSize, &installDir,
			  &installDirNameSize, sourceslistStr,
			  sourceslistStrSize, &packageFile, &packageFileSize,
			  &packageFileMemMap, &st, &numberOfPackageLists,
			  &gotonewline, &i, &packageList,
			  &packageListNameSizes, &packageListNameSize,
			  &execute, envp);
	  startCommandStrSize = 1;
	  startCommandStrSizeMax = 1;
	  startCommandStr = (char *) malloc (startCommandStrSize);
	  startCommandStrSize = 0;
	  installDirNameSize--;
	  appendInternal (&startCommandStr, &startCommandStrSize, &installDir,
			  &installDirNameSize, &startCommandStrSizeMax);
	  installDirNameSize++;
	  appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				  &varLocStr, &varLocStrSize,
				  &startCommandStrSizeMax);
	  appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				  &aGLocStr, &aGLocStrSize,
				  &startCommandStrSizeMax);
	  appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				  &statusLocStr, &statusLocStrSize,
				  &startCommandStrSizeMax);
	  startCommandStrSize++;
	  setupDstAppendInternal (&startCommandStr, &startCommandStrSize,
				  &startCommandStrSizeMax);
	  startCommandStr[startCommandStrSize - 1] = 0;
	  statusFile =
	    open (startCommandStr,
		  O_BINARY | O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	  fstat (statusFile, &st);
	  statusFileSize = st.st_size;
	  statusFileMemAlloc =
	    mmap (0, statusFileSize, PROT_READ, MAP_SHARED, statusFile, 0);
	  madvise (statusFileMemAlloc, statusFileSize, MADV_WILLNEED);
	  statusFileMemAllocPos = 0;
	  if (argc != 2)
	    {
	      packageNameStrsNum = argc - 2;
	      packageNameStrs = argv + 2;
	      packageNameStrSizes =
		malloc (packageNameStrsNum * sizeof (*packageNameStrSizes));
	      for (i = 0; i != packageNameStrsNum; i++)
		{
		  packageNameStrSizes[i] = strlen (packageNameStrs[i]);
		}
	    }
	  else
	    {
	      statusFileRequestSorted = true;
	      gotonewline = false;
	      firstiLIPE =
		(struct internalListInstalledPackageEntry *)
		malloc (sizeof (*firstiLIPE));
	      lastiLIPE = firstiLIPE;
	      lastiLIPE->packageNameStrSize = 0;
	      lastiLIPE->nextiLIPE = 0;
	      creatednew = false;
	      packageNameStrsNum = 0;
	      for (j = 0; j != statusFileSize; j++)
		{
		  if (gotonewline)
		    {
		      lastiLIPE->packageNameStrSize += creatednew;
		    }
		  else
		    {
		      creatednew = false;
		      if (!strncmp (statusFileMemAlloc + j, "Package: ", 9))
			{
			  j += 9;
			  creatednew = true;
			  if (lastiLIPE->nextiLIPE)
			    {
			      lastiLIPE = lastiLIPE->nextiLIPE;
			    }
			  lastiLIPE->packageNameStr = statusFileMemAlloc + j;
			  lastiLIPE->packageNameStrSize = 0;
			  packageNameStrsNum++;
			  lastiLIPE->nextiLIPE =
			    (struct internalListInstalledPackageEntry *)
			    malloc (sizeof (*lastiLIPE));
			}
		    }
		  gotonewline = statusFileMemAlloc[j] != '\n';
		}
	      free (lastiLIPE->nextiLIPE);
	      lastiLIPE->nextiLIPE = 0;
	      packageNameStrs =
		malloc (packageNameStrsNum * sizeof (*packageNameStrs));
	      packageNameStrSizes =
		malloc (packageNameStrsNum * sizeof (*packageNameStrSizes));
	      lastiLIPE = firstiLIPE;
	      i = 0;
	      while (lastiLIPE)
		{
		  packageNameStrs[i] = lastiLIPE->packageNameStr;
		  packageNameStrSizes[i] = lastiLIPE->packageNameStrSize;
		  i++;
		  lastiLIPE = lastiLIPE->nextiLIPE;
		  free (firstiLIPE);
		  firstiLIPE = lastiLIPE;
		}
	      if (firstiLIPE)
		{
		  free (firstiLIPE);
		}
	    }
	  startCommandStrSize--;
	  startCommandStrSize -= statusLocStrSize;
	  startCommandStrSize -= aGLocStrSize;
	  startCommandStrSize -= varLocStrSize;
	  appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				  &archivesLoc, &archivesLocSize,
				  &startCommandStrSizeMax);
	  printf ("Done\n");
	  printf ("Reading state information... ");
	  printf ("Done\n");
	  printf ("Calculating upgrade... ");
	  l = 0;
	  for (i = 0; i != packageNameStrsNum; i++)
	    {
	      gotonewline = false;
	      statusFileMemAllocPos = 0;
	      for (j = 0; j != statusFileSize; j++)
		{
		  if (statusFileMemAllocPos)
		    {
		      if (j - statusFileMemAllocPos == packageNameStrSizes[i])
			{
			  if (statusFileMemAlloc[j] == '\n')
			    {
			      j = statusFileSize - 1;
			    }
			  else
			    {
			      statusFileMemAllocPos = 0;
			    }
			}
		      else
			{
			  if (statusFileMemAlloc[j] !=
			      packageNameStrs[i][j - statusFileMemAllocPos])
			    {
			      statusFileMemAllocPos = 0;
			    }
			}
		    }
		  if (!gotonewline)
		    {
		      if (!strncmp (statusFileMemAlloc + j, "Package: ", 9))
			{
			  j += 9;
			  statusFileMemAllocPos = j;
			}
		    }
		  gotonewline = statusFileMemAlloc[j] != '\n';
		}
	      gotonewline = false;
	      commitHash = 0;
	      commitHashSize = 0;
	      for (j = statusFileMemAllocPos; j != statusFileSize; j++)
		{
		  if (!gotonewline)
		    {
		      if (!strncmp
			  (statusFileMemAlloc + j, "CommitHash: ", 12))
			{
			  j += 12;
			  commitHash = statusFileMemAlloc + j;
			  statusFileMemAllocPos = j;
			}
		      else
			if (!strncmp (statusFileMemAlloc + j, "Package: ", 9))
			{
			  j = statusFileSize - 1;
			}
		    }
		  gotonewline = statusFileMemAlloc[j] != '\n';
		  if (commitHash)
		    {
		      commitHashSize += gotonewline;
		      if (!gotonewline)
			{
			  j = statusFileSize - 1;
			}
		    }
		}
	      appendInternal (&startCommandStr, &startCommandStrSize,
			      packageNameStrs + i, packageNameStrSizes + i,
			      &startCommandStrSizeMax);
	      appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				      &gitLocStr, &gitLocStrSize,
				      &startCommandStrSizeMax);
	      appendInternalSrcConst (&startCommandStr, &startCommandStrSize,
				      &HEADLocStr, &HEADLocStrSize,
				      &startCommandStrSizeMax);
	      startCommandStrSize++;
	      setupDstAppendInternal (&startCommandStr, &startCommandStrSize,
				      &startCommandStrSizeMax);
	      startCommandStr[startCommandStrSize - 1] = 0;
	      headFile = open (startCommandStr, O_BINARY | O_RDWR);
	      fstat (headFile, &st);
	      headFileSize = st.st_size;
	      headFileMemMap =
		mmap (0, headFileSize, PROT_READ, MAP_SHARED, headFile, 0);
	      madvise (headFileMemMap, headFileSize, MADV_WILLNEED);
	      startCommandStrSize--;
	      commitHead = 0;
	      commitHeadSize = 0;
	      for (j = 0; j != headFileSize; j++)
		{
		  if (headFileMemMap[j] == 'r' && headFileMemMap[j + 1] == 'e'
		      && headFileMemMap[j + 2] == 'f'
		      && headFileMemMap[j + 3] == ':'
		      && headFileMemMap[j + 4] == ' ')
		    {
		      j += 5;
		      commitHead = headFileMemMap + j;
		      for (commitHeadSize = 0;
			   commitHead[commitHeadSize] != '\n'
			   && j + commitHeadSize != headFileSize;
			   commitHeadSize++);
		      j = headFileSize - 1;
		    }
		}
	      startCommandStrSize -= HEADLocStrSize;
	      remoteHash = 0;
	      remoteHashSize = 0;
	      if (commitHeadSize)
		{
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &configLocStr,
					  &configLocStrSize,
					  &startCommandStrSizeMax);
		  startCommandStrSize++;
		  setupDstAppendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &startCommandStrSizeMax);
		  startCommandStr[startCommandStrSize - 1] = 0;
		  configFile =
		    open (startCommandStr,
			  O_BINARY | O_RDONLY | O_CREAT | O_APPEND,
			  S_IRUSR | S_IWUSR);
		  startCommandStrSize -= configLocStrSize + 1;
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize,
					  &remotesLocStr, &remotesLocStrSize,
					  &startCommandStrSizeMax);
		  fstat (configFile, &st);
		  configFileSize = st.st_size;
		  configFileMemMap =
		    mmap (0, configFileSize, PROT_READ, MAP_SHARED,
			  configFile, 0);
		  madvise (configFileMemMap, configFileSize, MADV_WILLNEED);
		  configFileMemMapPos = 0;
		  remoteLoc = 0;
		  remoteLocSize = 0;
		  for (j = 0; j != configFileSize; j++)
		    {
		      if (!strncmp
			  (configFileMemMap + j, commitHead, commitHeadSize))
			{
			  for (; j != 0; j--)
			    {
			      if (!strncmp (configFileMemMap + j, "merge", 5))
				{
				  configFileMemMapPos = j;
				  j = 1;
				}
			      if (configFileMemMap[j] == '\n')
				{
				  j = 1;
				}
			    }
			  for (j = configFileMemMapPos; j != 0; j--)
			    {
			      if (configFileMemMap[j] == '[')
				{
				  configFileMemMapPos = j;
				  j = 1;
				}
			    }
			  for (j = configFileMemMapPos; j != configFileSize;
			       j++)
			    {
			      if (!strncmp
				  (configFileMemMap + j, "remote", 6))
				{
				  configFileMemMapPos = j + 6;
				  j = configFileSize - 1;
				}
			    }
			  for (j = configFileMemMapPos; j != configFileSize;
			       j++)
			    {
			      if (configFileMemMap[j] != ' ')
				{
				  configFileMemMapPos = j;
				  j = configFileSize - 1;
				}
			    }
			  if (configFileMemMapPos != configFileSize
			      && configFileMemMap[configFileMemMapPos] == '=')
			    {
			      for (j = configFileMemMapPos + 1;
				   j != configFileSize; j++)
				{
				  if (configFileMemMap[j] != ' ')
				    {
				      configFileMemMapPos = j;
				      j = configFileSize - 1;
				    }
				}
			      for (j = configFileMemMapPos;
				   j != configFileSize; j++)
				{
				  remoteLocSize++;
				  if (configFileMemMap[j] == '\n')
				    {
				      remoteLocSize--;
				      j = configFileSize - 1;
				    }
				}
			      remoteLoc =
				configFileMemMap + configFileMemMapPos;
			    }
			  else
			    {
			      configFileMemMapPos = 0;
			    }
			  j = configFileSize - 1;
			}
		    }
		  if (remoteLocSize)
		    {
		      appendInternal (&startCommandStr, &startCommandStrSize,
				      &remoteLoc, &remoteLocSize,
				      &startCommandStrSizeMax);
		      configFileMemMapPos = 0;
		      for (j = commitHeadSize; j != 0; j--)
			{
			  if (commitHead[j] == '/')
			    {
			      configFileMemMapPos = j;
			      j = 1;
			    }
			}
		      commitHead += configFileMemMapPos;
		      commitHeadSize -= configFileMemMapPos;
		      appendInternal (&startCommandStr, &startCommandStrSize,
				      &commitHead, &commitHeadSize,
				      &startCommandStrSizeMax);
		      startCommandStrSize++;
		      setupDstAppendInternal (&startCommandStr,
					      &startCommandStrSize,
					      &startCommandStrSizeMax);
		      startCommandStr[startCommandStrSize - 1] = 0;
		      fd = open (startCommandStr, O_BINARY | O_RDWR);
		      if (fd != -1)
			{
			  munmap (headFileMemMap, headFileSize);
			  close (headFile);
			  headFile = fd;
			  fstat (headFile, &st);
			  headFileSize = st.st_size;
			  headFileMemMap =
			    mmap (0, headFileSize, PROT_READ, MAP_SHARED,
				  headFile, 0);
			  madvise (headFileMemMap, headFileSize,
				   MADV_WILLNEED);
			  remoteHash = headFileMemMap;
			  for (remoteHashSize = 0;
			       remoteHash[remoteHashSize] != '\n'
			       && remoteHashSize != headFileSize;
			       remoteHashSize++);
			  startCommandStrSize -=
			    remotesLocStrSize + remoteLocSize +
			    commitHeadSize + 1;
			}
		      else
			{
			  startCommandStrSize -=
			    remotesLocStrSize + remoteLocSize +
			    commitHeadSize + 1;
			  appendInternalSrcConst (&startCommandStr,
						  &startCommandStrSize,
						  &packedRefsLocStr,
						  &packedRefsLocStrSize,
						  &startCommandStrSizeMax);
			  startCommandStrSize++;
			  setupDstAppendInternal (&startCommandStr,
						  &startCommandStrSize,
						  &startCommandStrSizeMax);
			  startCommandStr[startCommandStrSize - 1] = 0;
			  fd = open (startCommandStr, O_BINARY | O_RDWR);
			  startCommandStrSize -= packedRefsLocStrSize + 1;
			  appendInternal (&startCommandStr,
					  &startCommandStrSize, &commitHead,
					  &commitHeadSize,
					  &startCommandStrSizeMax);
			  startCommandStrSize -= commitHeadSize;
			  commitHead = startCommandStr + startCommandStrSize;
			  munmap (headFileMemMap, headFileSize);
			  close (headFile);
			  headFile = fd;
			  fstat (headFile, &st);
			  headFileSize = st.st_size;
			  headFileMemMap =
			    mmap (0, headFileSize, PROT_READ, MAP_SHARED,
				  headFile, 0);
			  madvise (headFileMemMap, headFileSize,
				   MADV_WILLNEED);
			  for (j = 0; j != headFileSize; j++)
			    {
			      if (headFileMemMap[j] == '\n')
				{
				  remoteHashSize = j + 1;
				}
			      for (k = 0;
				   j + k != headFileSize
				   && k != remotesLocStrSize
				   && headFileMemMap[j + k] ==
				   remotesLocStr[k]; k++);
			      j += k;
			      if (k == remotesLocStrSize)
				{
				  for (k = 0;
				       j + k != headFileSize
				       && k != remoteLocSize
				       && headFileMemMap[j + k] ==
				       remoteLoc[k]; k++);
				  j += k;
				  if (k == remoteLocSize)
				    {
				      for (k = 0;
					   j + k != headFileSize
					   && k != commitHeadSize
					   && headFileMemMap[j + k] ==
					   commitHead[k]; k++);
				      if (k == commitHeadSize)
					{
					  remoteHash =
					    headFileMemMap + remoteHashSize;
					  remoteHashSize =
					    headFileSize - remoteHashSize;
					  j = headFileSize - 1;
					}
				    }
				}
			    }
			}
		    }
		}
	      startCommandStrSize -= packageNameStrSizes[i] + gitLocStrSize;
	      if (commitHeadSize && remoteHash
		  && commitHashSize <= remoteHashSize)
		{
		  if (strncmp (commitHash, remoteHash, commitHashSize))
		    {
		      packageNameStrs[l] = packageNameStrs[i];
		      packageNameStrSizes[l] = packageNameStrSizes[i];
		      l++;
		    }
		}
	      munmap (headFileMemMap, headFileSize);
	      close (headFile);
	    }
	  packageNameStrsNum = l;
	  printf ("Done\n");
	  printf ("The following package will be upgraded:\n");
	  printf (" ");
	  for (i = 0; i != packageNameStrsNum; i++)
	    {
	      packageNameStrSize = 1;
	      packageNameStrSize <<= sizeof (fieldPrecisionSpecifier) * 8 - 1;
	      packageNameStrSize--;
	      fieldPrecisionSpecifier = packageNameStrSize;
	      packageNameStrSize = packageNameStrSizes[i];
	      printf (" ");
	      while (packageNameStrSize > fieldPrecisionSpecifier)
		{
		  printf ("%.*s", fieldPrecisionSpecifier,
			  packageNameStrs[i] + packageNameStrSizes[i] -
			  packageNameStrSize);
		  packageNameStrSize -= fieldPrecisionSpecifier;
		}
	      fieldPrecisionSpecifier = packageNameStrSize;
	      printf ("%.*s", fieldPrecisionSpecifier,
		      packageNameStrs[i] + packageNameStrSizes[i] -
		      packageNameStrSize);
	    }
	  printf ("\n");
	  printf
	    ("%lld upgraded, 0 newly installed, 0 to remove and 0 not upgraded.\n",
	     packageNameStrsNum);
	  printf ("Do you want to continue? [Y/n] ");
	  fflush (stdout);
	  if (getchar () != 'n')
	    {
	      *sargv = 0;
	      sargv[1] = "-C";
	      sargv[3] = "merge";
	      sargv[4] = 0;
	      for (i = 0; i != packageNameStrsNum; i++)
		{
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  packageNameStrs + i,
				  packageNameStrSizes + i,
				  &startCommandStrSizeMax);
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &gitLocStr,
					  &gitLocStrSize,
					  &startCommandStrSizeMax);
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &configLocStr,
					  &configLocStrSize,
					  &startCommandStrSizeMax);
		  startCommandStrSize++;
		  setupDstAppendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &startCommandStrSizeMax);
		  startCommandStr[startCommandStrSize - 1] = 0;
		  configFile =
		    open (startCommandStr,
			  O_BINARY | O_RDONLY | O_CREAT | O_APPEND,
			  S_IRUSR | S_IWUSR);
		  fstat (configFile, &st);
		  configFileSize = st.st_size;
		  configFileMemMap =
		    mmap (0, configFileSize, PROT_READ, MAP_SHARED,
			  configFile, 0);
		  madvise (configFileMemMap, configFileSize, MADV_WILLNEED);
		  configFileMemMapPos = 0;
		  for (j = 0; j != configFileSize; j++)
		    {
		      if (configFileMemMap[j] == 'u'
			  && configFileMemMap[j + 1] == 'r'
			  && configFileMemMap[j + 2] == 'l'
			  && configFileMemMap[j + 3] == ' '
			  && configFileMemMap[j + 4] == '='
			  && configFileMemMap[j + 5] == ' ')
			{
			  configFileMemMapPos = j + 6;
			  j = configFileSize - 1;
			}
		    }
		  startCommandStrSize -= gitLocStrSize;
		  startCommandStrSize -= configLocStrSize;
		  startCommandStrSize--;
		  for (j = 0; j != numberOfPackageLists; j++)
		    {
		      if (configFileMemMapPos + packageListNameSizes[j] <=
			  configFileSize)
			{
			  if (!strncmp
			      (configFileMemMap + configFileMemMapPos,
			       packageList[j], packageListNameSizes[j] - 1))
			    {
			      packageNameStrSize = 1;
			      packageNameStrSize <<=
				sizeof (fieldPrecisionSpecifier) * 8 - 1;
			      packageNameStrSize--;
			      fieldPrecisionSpecifier = packageNameStrSize;
			      packageNameStrSize = packageNameStrSizes[i];
			      printf ("Get:1 %s main amd64 ", packageList[j]);
			      while (packageNameStrSize >
				     fieldPrecisionSpecifier)
				{
				  printf ("%.*s", fieldPrecisionSpecifier,
					  packageNameStrs[i] +
					  packageNameStrSizes[i] -
					  packageNameStrSize);
				  packageNameStrSize -=
				    fieldPrecisionSpecifier;
				}
			      fieldPrecisionSpecifier = packageNameStrSize;
			      printf ("%.*s", fieldPrecisionSpecifier,
				      packageNameStrs[i] +
				      packageNameStrSizes[i] -
				      packageNameStrSize);
			      printf (" [0 kB]\n");
			      startCommandStrSize++;
			      setupDstAppendInternal (&startCommandStr,
						      &startCommandStrSize,
						      &startCommandStrSizeMax);
			      startCommandStr[startCommandStrSize - 1] = 0;
			      sargv[2] = startCommandStr;
			      if (*sargv)
				{
				  waitid (P_PID, spid, &ssiginfo, WEXITED);
				}
			      if (sargv[2])
				{
				  *sargv = "git";
				}
			      if (*sargv
				  && posix_spawnp (&spid, *sargv, NULL, NULL,
						   sargv, envp) != 0)
				{
				  *sargv = 0;
				}
			      startCommandStrSize--;
			      j = numberOfPackageLists - 1;
			    }
			}
		    }
		  startCommandStrSize -= packageNameStrSizes[i];
		  close (configFile);
		  munmap (configFileMemMap, configFileSize);
		}
	      newNameStr = malloc (1);
	      newNameStrSize = 0;
	      newNameStrSizeMax = 1;
	      tmpNewNameStr = malloc (1);
	      tmpNewNameStrSize = 0;
	      tmpNewNameStrSizeMax = 1;
	      sargv[3] = "checkout";
	      for (i = 0; i != packageNameStrsNum; i++)
		{
		  packageNameStrSize = 1;
		  packageNameStrSize <<= sizeof (fieldPrecisionSpecifier) *
		    8 - 1;
		  packageNameStrSize--;
		  fieldPrecisionSpecifier = packageNameStrSize;
		  packageNameStrSize = packageNameStrSizes[i];
		  if (*sargv)
		    {
		      waitid (P_PID, spid, &ssiginfo, WEXITED);
		    }
		  printf ("Preparing to unpack \"");
		  while (packageNameStrSize > fieldPrecisionSpecifier)
		    {
		      printf ("%.*s", fieldPrecisionSpecifier,
			      packageNameStrs[i] + packageNameStrSizes[i] -
			      packageNameStrSize);
		      packageNameStrSize -= fieldPrecisionSpecifier;
		    }
		  fieldPrecisionSpecifier = packageNameStrSize;
		  printf ("%.*s\" ...\n", fieldPrecisionSpecifier,
			  packageNameStrs[i] + packageNameStrSizes[i] -
			  packageNameStrSize);
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  packageNameStrs + i,
				  packageNameStrSizes + i,
				  &startCommandStrSizeMax);
		  startCommandStrSize++;
		  setupDstAppendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &startCommandStrSizeMax);
		  startCommandStr[startCommandStrSize - 1] = 0;
		  packageNameStrSize = 1;
		  packageNameStrSize <<= sizeof (fieldPrecisionSpecifier) *
		    8 - 1;
		  packageNameStrSize--;
		  fieldPrecisionSpecifier = packageNameStrSize;
		  packageNameStrSize = packageNameStrSizes[i];
		  printf ("Unpacking ");
		  while (packageNameStrSize > fieldPrecisionSpecifier)
		    {
		      printf ("%.*s", fieldPrecisionSpecifier,
			      packageNameStrs[i] + packageNameStrSizes[i] -
			      packageNameStrSize);
		      packageNameStrSize -= fieldPrecisionSpecifier;
		    }
		  fieldPrecisionSpecifier = packageNameStrSize;
		  printf ("%.*s ...\n", fieldPrecisionSpecifier,
			  packageNameStrs[i] + packageNameStrSizes[i] -
			  packageNameStrSize);
		  sargv[2] = startCommandStr;
		  fflush (stdout);
		  if (sargv[2])
		    {
		      *sargv = "git";
		    }
		  if (*sargv
		      && posix_spawnp (&spid, *sargv, NULL, NULL, sargv,
				       envp) != 0)
		    {
		      *sargv = 0;
		    }
		  startCommandStrSize--;
		  startCommandStrSize -= packageNameStrSizes[i];
		}
	      fstat (statusFile, &st);
	      statusFileSize = st.st_size;
	      munmap (statusFileMemAlloc, statusFileSize);
	      statusFileMemAlloc =
		mmap (0, statusFileSize, PROT_READ | PROT_WRITE, MAP_SHARED,
		      statusFile, 0);
	      madvise (statusFileMemAlloc, statusFileSize, MADV_WILLNEED);
	      statusFileMemAllocPos = 0;
	      selectedLength1 = 0;
	      selectedLength2 = 0;
	      statusFileRangesPos = 0;
	      statusFileAddLocPos = 0;
	      statusFileAddStrPos = 0;
	      statusFileRangesSize = 0;
	      statusFileAddLocSize = 0;
	      statusFileAddStrSize = 0;
	      statusFileRangesPosMax = 0;
	      statusFileAddLocPosMax = 0;
	      statusFileRangesSizeMax = 0;
	      statusFileAddLocSizeMax = 0;
	      statusFileAddStrSizeMax = 0;
	      *statusFileRanges = (size_t *) malloc (sizeof (size_t) * 2);
	      *statusFileAddLoc = (size_t *) malloc (sizeof (size_t));
	      *statusFileAddSize = (size_t *) malloc (sizeof (size_t));
	      *statusFileAddStr = (char *) malloc (sizeof (char));
	      statusFileRanges[0][0] = statusFileRanges[0][1] =
		statusFileAddLoc[0][0] = statusFileAddSize[0][0] = 0;
	      newstatusFileSize = statusFileSize;
	      statusFileAddStrLocPos = 0;
	      shellLocStr = getenv ("SHELL");
	      shellLocStrSize = strlen (shellLocStr);
	      newNameStrSizeMax = 1;
	      newNameStr = (char *) malloc (newNameStrSizeMax);
	      newNameStrSize = 0;
	      shellSpace = true;
	      for (j = 0; j != shellLocStrSize; j++)
		{
		  shellSpace &= shellLocStr[j] != ' ';
		}
	      shellSpace = 1 - shellSpace;
	      for (i = 0; i != packageNameStrsNum; i++)
		{
		  packageNameStrSize = 1;
		  packageNameStrSize <<= sizeof (fieldPrecisionSpecifier) *
		    8 - 1;
		  packageNameStrSize--;
		  fieldPrecisionSpecifier = packageNameStrSize;
		  packageNameStrSize = packageNameStrSizes[i];
		  if (*sargv)
		    {
		      waitid (P_PID, spid, &ssiginfo, WEXITED);
		    }
		  *sargv = 0;
		  printf ("Setting up ");
		  while (packageNameStrSize > fieldPrecisionSpecifier)
		    {
		      printf ("%.*s", fieldPrecisionSpecifier,
			      packageNameStrs[i] + packageNameStrSizes[i] -
			      packageNameStrSize);
		      packageNameStrSize -= fieldPrecisionSpecifier;
		    }
		  fieldPrecisionSpecifier = packageNameStrSize;
		  printf ("%.*s ...\n", fieldPrecisionSpecifier,
			  packageNameStrs[i] + packageNameStrSizes[i] -
			  packageNameStrSize);
		  startCommandStrSize =
		    cdStrSize + (installDirNameSize + archivesLocSize +
				 packageNameStrSizes[i]) * 2 +
		    buildLocStrSize + commandSucessStrSize + shellLocStrSize +
		    shellOptionRunStrSize;
		  newNameStrSize = 0;
		  installDirNameSize--;
		  appendInternal (&newNameStr, &newNameStrSize, &installDir,
				  &installDirNameSize, &newNameStrSizeMax);
		  installDirNameSize++;
		  appendInternalSrcConst (&newNameStr, &newNameStrSize,
					  &configurationLoc,
					  &configurationLocSize,
					  &newNameStrSizeMax);
		  appendInternalSrcConst (&newNameStr, &newNameStrSize,
					  &overrideicLoc, &overrideicLocSize,
					  &newNameStrSizeMax);
		  appendInternal (&newNameStr, &newNameStrSize,
				  &packageNameStrs[i],
				  &packageNameStrSizes[i],
				  &newNameStrSizeMax);
		  appendInternalSrcConst (&newNameStr, &newNameStrSize,
					  &directorySeparator,
					  &directorySeparatorSize,
					  &newNameStrSizeMax);
		  appendInternalSrcConst (&newNameStr, &newNameStrSize,
					  &runshLocStr, &runshLocStrSize,
					  &newNameStrSizeMax);
		  newNameStrSize++;
		  setupDstAppendInternal (&newNameStr, &newNameStrSize,
					  &newNameStrSizeMax);
		  newNameStr[newNameStrSize - 1] = 0;
		  setupDstAppendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &startCommandStrSizeMax);
		  sargv[1] = "-c";
		  sargv[3] = 0;
		  startCommandStrSize = 0;
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &cdStr,
					  &cdStrSize,
					  &startCommandStrSizeMax);
		  installDirNameSize--;
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  &installDir, &installDirNameSize,
				  &startCommandStrSizeMax);
		  installDirNameSize++;
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &archivesLoc,
					  &archivesLocSize,
					  &startCommandStrSizeMax);
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  &packageNameStrs[i],
				  &packageNameStrSizes[i],
				  &startCommandStrSizeMax);
		  startCommandStrPos = startCommandStrSize;
		  runFile =
		    open (newNameStr,
			  O_BINARY | O_RDONLY | O_CREAT | O_APPEND,
			  S_IRUSR | S_IWUSR);
		  runFileMemAllocPos = 0;
		  runFileLine = 0;
		  runconfig = -1;
		  runconfigSize = runconfigMemAllocPos = 0;
		  runconfigMemAlloc = MAP_FAILED;
		  config = -1;
		  configSize = configMemAllocPos = 0;
		  configMemAlloc = MAP_FAILED;
		  gotonewline = false;
		  newNameStrSize -=
		    overrideicLocSize + packageNameStrSizes[i] +
		    directorySeparatorSize + runshLocStrSize + 1;
		  if (runFile != -1)
		    {
		      fstat (runFile, &st);
		      runFileSize = st.st_size;
		      runFileMemAlloc =
			mmap (0, runFileSize, PROT_READ, MAP_SHARED, runFile,
			      0);
		      madvise (runFileMemAlloc, runFileSize, MADV_WILLNEED);
		      execute = false;
		      for (j = 0; j != runFileSize; j++)
			{
			  if (runFileMemAlloc[j] == '\r')
			    {
			      j += j + 1 != runFileSize;
			    }
			  if (!gotonewline)
			    {
			      runFileLine++;
			      appendInternalSrcConst (&newNameStr,
						      &newNameStrSize,
						      &overrideicLoc,
						      &overrideicLocSize,
						      &newNameStrSizeMax);
			      appendInternal (&newNameStr, &newNameStrSize,
					      &packageNameStrs[i],
					      &packageNameStrSizes[i],
					      &newNameStrSizeMax);
			      appendInternalSrcConst (&newNameStr,
						      &newNameStrSize,
						      &directorySeparator,
						      &directorySeparatorSize,
						      &newNameStrSizeMax);
			      appendInternalSrcConst (&newNameStr,
						      &newNameStrSize,
						      &runconfigLocStr,
						      &runconfigLocStrSize,
						      &newNameStrSizeMax);
			      lPos1 = runFileLine;
			      for (lSize1 = 0; lPos1; lSize1++)
				{
				  newNameStrSize++;
				  setupDstAppendInternal (&newNameStr,
							  &newNameStrSize,
							  &newNameStrSizeMax);
				  newNameStr[newNameStrSize - 1] =
				    '0' + lPos1 % 10;
				  lPos1 /= 10;
				}
			      for (; lPos1 != lSize1 >> 1; lPos1++)
				{
				  k =
				    newNameStr[newNameStrSize + lPos1 -
					       lSize1];
				  newNameStr[newNameStrSize + lPos1 -
					     lSize1] =
				    newNameStr[newNameStrSize - 1 - lPos1];
				  newNameStr[newNameStrSize - lPos1 - 1] = k;
				}
			      newNameStrSize++;
			      setupDstAppendInternal (&newNameStr,
						      &newNameStrSize,
						      &newNameStrSizeMax);
			      newNameStr[newNameStrSize - 1] = 0;
			      runconfig =
				open (newNameStr,
				      O_BINARY | O_RDWR | O_APPEND,
				      S_IRUSR | S_IWUSR);
			      runconfigMemAllocPos = 0;
			      fstat (runconfig, &st);
			      runconfigSize = st.st_size;
			      runconfigMemAlloc =
				mmap (0, runconfigSize, PROT_READ, MAP_SHARED,
				      runconfig, 0);
			      nQuotes = 0;
			      runFileMemAllocPos = j;
			      execute = j != runFileSize;
			      while (!nQuotes)
				{
				  for (;
				       j != runFileSize
				       && runFileMemAlloc[j] == ' '; j++);
				  for (;
				       j != runFileSize
				       && runFileMemAlloc[j] == '\"'; j++)
				    {
				      nQuotes++;
				    }
				  execute = j != runFileSize;
				  execute = execute
				    && runFileMemAlloc[j] == 'c';
				  j += execute;
				  if (execute)
				    {
				      for (;
					   j != runFileSize
					   && runFileMemAlloc[j] == '\"'; j++)
					{
					  nQuotes++;
					}
				    }
				  execute = execute && j != runFileSize;
				  execute = execute
				    && runFileMemAlloc[j] == 'd';
				  if (!nQuotes)
				    {
				      if (runFileMemAlloc[j] == '=')
					{
					  break;
					}
				      else
					{
					  for (;
					       j != runFileSize
					       && runFileMemAlloc[j] != '='
					       && runFileMemAlloc[j] != ' '
					       && runFileMemAlloc[j] != '\"'
					       &&
					       !isnewlinechar (runFileMemAlloc
							       [j]); j++);
					  if (j == runFileSize
					      || runFileMemAlloc[j] == ' '
					      || runFileMemAlloc[j] == '\"'
					      ||
					      isnewlinechar (runFileMemAlloc
							     [j]))
					    {
					      break;
					    }
					  if (runFileMemAlloc[j] == '=')
					    {
					      for (;
						   j != runFileSize
						   && runFileMemAlloc[j] !=
						   ' '
						   &&
						   !isnewlinechar
						   (runFileMemAlloc[j]); j++)
						{
						  if (runFileMemAlloc[j] ==
						      '\"')
						    {
						      for (;
							   j != runFileSize
							   &&
							   runFileMemAlloc[j]
							   != '\"'; j++);
						    }
						}
					      nQuotes = j == runFileSize;
					    }
					}
				    }
				}
			      j += execute && j != runFileSize
				&& runFileMemAlloc[j] == 'd';
			      if (execute)
				{
				  for (;
				       j != runFileSize
				       && runFileMemAlloc[j] == '\"'; j++)
				    {
				      nQuotes++;
				    }
				}
			      execute = execute && 1 - (nQuotes & 1);
			      execute = execute && j != runFileSize;
			      if (execute)
				{
				  for (;
				       j != runFileSize
				       && runFileMemAlloc[j] == ' '; j++);
				}
			      execute = 1 - execute;
			      execute = execute && j != runFileSize;
			      if (execute)
				{
				  j = runFileMemAllocPos;
				  appendInternalSrcConst (&startCommandStr,
							  &startCommandStrSize,
							  &commandSucessStr,
							  &commandSucessStrSize,
							  &startCommandStrSizeMax);
				}
			      else
				{
				  while (j != runFileSize
					 &&
					 !isnewlinechar (runFileMemAlloc[j])
					 && !(runFileMemAlloc[j] == '\r'
					      && j + 1 != runFileSize
					      &&
					      isnewlinechar (runFileMemAlloc
							     [j + 1])))
				    {
				      for (;
					   j != runFileSize
					   && (runFileMemAlloc[j] == '/'
					       || runFileMemAlloc[j] == '\"');
					   j++)
					{
					  nQuotes +=
					    runFileMemAlloc[j] == '\"';
					}
				      for (;
					   j != runFileSize
					   && (runFileMemAlloc[j] == '\\'
					       || runFileMemAlloc[j] == '\"');
					   j++)
					{
					  nQuotes +=
					    runFileMemAlloc[j] == '\"';
					}
				      if (1 - (nQuotes & 1)
					  && j != runFileSize
					  && runFileMemAlloc[j] == ' ')
					{
					  break;
					}
				      if (runFileMemAlloc[j] == '.')
					{
					  if (j + 1 != runFileSize
					      && runFileMemAlloc[j + 1] ==
					      '.')
					    {
					      if (j + 2 != runFileSize
						  && (runFileMemAlloc[j + 2]
						      == '/'
						      || runFileMemAlloc[j +
									 2] ==
						      '\\'
						      ||
						      isnewlinechar
						      (runFileMemAlloc[j + 2])
						      ||
						      (runFileMemAlloc[j + 2]
						       == '\r'
						       && j + 3 != runFileSize
						       &&
						       isnewlinechar
						       (runFileMemAlloc
							[j + 3]))))
						{
						  for (;
						       startCommandStrSize !=
						       cdStrSize
						       &&
						       startCommandStr
						       [startCommandStrSize]
						       != '/'
						       &&
						       startCommandStr
						       [startCommandStrSize]
						       != '\\';
						       startCommandStrSize--);
						  execute = true;
						}
					    }
					  else if (j + 1 != runFileSize
						   && (runFileMemAlloc[j + 1]
						       == '/'
						       || runFileMemAlloc[j +
									  1]
						       == '\\'
						       ||
						       isnewlinechar
						       (runFileMemAlloc
							[j + 1])
						       ||
						       (runFileMemAlloc[j + 1]
							== '\r'
							&& j + 2 !=
							runFileSize
							&&
							isnewlinechar
							(runFileMemAlloc
							 [j + 2]))))
					    {
					      execute = true;
					    }
					}
				      if (execute)
					{
					  for (;
					       j != runFileSize
					       && runFileMemAlloc[j] == '.';
					       j++);
					}
				      else
					{
					  appendInternalSrcConst
					    (&startCommandStr,
					     &startCommandStrSize,
					     &directorySeparator,
					     &directorySeparatorSize,
					     &startCommandStrSizeMax);
					  for (;
					       j != runFileSize
					       && !(1 - (nQuotes & 1)
						    && runFileMemAlloc[j] ==
						    ' ')
					       && runFileMemAlloc[j] != '/'
					       && runFileMemAlloc[j] != '\\'
					       &&
					       !isnewlinechar (runFileMemAlloc
							       [j])
					       && !(runFileMemAlloc[j] == '\r'
						    && j + 1 != runFileSize
						    &&
						    isnewlinechar
						    (runFileMemAlloc[j + 1]));
					       j++)
					    {
					      if (runFileMemAlloc[j] == '\"')
						{
						  nQuotes++;
						}
					      else
						{
						  startCommandStrSize++;
						  setupDstAppendInternal
						    (&startCommandStr,
						     &startCommandStrSize,
						     &startCommandStrSizeMax);
						  startCommandStr
						    [startCommandStrSize -
						     1] = runFileMemAlloc[j];
						}
					    }
					}
				      execute = false;
				    }
				  startCommandStrPos = startCommandStrSize;
				}
			      newNameStrSize -=
				runconfigLocStrSize + lSize1 + 1;
			      configMemAlloc = runconfigMemAlloc;
			      if (runconfigMemAlloc != MAP_FAILED)
				{
				  madvise (runconfigMemAlloc, runconfigSize,
					   MADV_WILLNEED);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &configLocStr,
							  &configLocStrSize,
							  &newNameStrSizeMax);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &directorySeparator,
							  &directorySeparatorSize,
							  &newNameStrSizeMax);
				  appendInternal (&newNameStr,
						  &newNameStrSize,
						  &runconfigMemAlloc,
						  &runconfigSize,
						  &newNameStrSizeMax);
				  newNameStrSize++;
				  setupDstAppendInternal (&newNameStr,
							  &newNameStrSize,
							  &newNameStrSizeMax);
				  newNameStr[newNameStrSize - 1] = 0;
				  newNameStrSize -=
				    configLocStrSize +
				    directorySeparatorSize + runconfigSize +
				    1;
				  munmap (runconfigMemAlloc, runconfigSize);
				  config =
				    open (newNameStr,
					  O_BINARY | O_RDWR | O_APPEND,
					  S_IRUSR | S_IWUSR);
				  configMemAllocPos = 0;
				  fstat (config, &st);
				  configSize = st.st_size;
				  configMemAlloc =
				    mmap (0, configSize, PROT_READ,
					  MAP_SHARED, config, 0);
				  if (configMemAlloc != MAP_FAILED)
				    {
				      madvise (configMemAlloc, configSize,
					       MADV_WILLNEED);
				    }
				}
			      newNameStrSize -=
				overrideicLocSize + packageNameStrSizes[i] +
				directorySeparatorSize;
			      gotonewline = true;
			    }
			  if (isnewlinechar (runFileMemAlloc[j]))
			    {
			      gotonewline = false;
			    }
			  if (execute && gotonewline)
			    {
			      startCommandStrSize++;
			      setupDstAppendInternal (&startCommandStr,
						      &startCommandStrSize,
						      &startCommandStrSizeMax);
			      startCommandStr[startCommandStrSize - 1] =
				runFileMemAlloc[j];
			    }
			  else if (execute)
			    {
			      if (configMemAlloc != MAP_FAILED)
				{
				  for (quoteType = nQuotes =
				       configMemAllocPos = 0;
				       configMemAllocPos != configSize;
				       configMemAllocPos++)
				    {
				      if (!gotonewline)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] = ' ';
					  execute = gotonewline = true;
					}
				      if (isnewlinechar
					  (configMemAlloc[configMemAllocPos]))
					{
					  gotonewline = false;
					}
				      if (configMemAlloc[configMemAllocPos] ==
					  '\r'
					  && configMemAllocPos + 1 !=
					  configSize
					  &&
					  isnewlinechar (configMemAlloc
							 [configMemAllocPos +
							  1]))
					{
					  configMemAllocPos++;
					  gotonewline = false;
					}
				      if (execute
					  && configMemAlloc[configMemAllocPos]
					  == '#' && (!configMemAllocPos
						     ||
						     configMemAlloc
						     [configMemAllocPos -
						      1] == ' '
						     ||
						     isnewlinechar
						     (configMemAlloc
						      [configMemAllocPos -
						       1]))
					  && 1 - (nQuotes & 1))
					{
					  execute = false;
					}
				      if ((execute
					   &&
					   configMemAlloc[configMemAllocPos]
					   == '\"')
					  || configMemAlloc[configMemAllocPos]
					  == '\'')
					{
					  if (1 - (nQuotes & 1))
					    {
					      quoteType =
						configMemAlloc
						[configMemAllocPos];
					    }
					  nQuotes +=
					    quoteType ==
					    configMemAlloc[configMemAllocPos];
					}
				      if (gotonewline & execute)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] =
					    configMemAlloc[configMemAllocPos];
					}
				    }
				}
			      startCommandStrSize++;
			      setupDstAppendInternal (&startCommandStr,
						      &startCommandStrSize,
						      &startCommandStrSizeMax);
			      startCommandStr[startCommandStrSize - 1] = 0;
			      sargv[2] = startCommandStr;
			      if (*sargv)
				{
				  waitid (P_PID, spid, &ssiginfo, WEXITED);
				}
			      if (sargv[2])
				{
				  *sargv = shellLocStr;
				}
			      if (*sargv
				  && posix_spawnp (&spid, *sargv, NULL, NULL,
						   sargv, envp) != 0)
				{
				  *sargv = 0;
				}
			      startCommandStrSize = startCommandStrPos;
			      execute = true;
			      gotonewline = false;
			    }
			}
		      if (execute)
			{
			  if (configMemAlloc != MAP_FAILED)
			    {
			      for (quoteType = nQuotes = configMemAllocPos =
				   0; configMemAllocPos != configSize;
				   configMemAllocPos++)
				{
				  if (!gotonewline)
				    {
				      startCommandStrSize++;
				      setupDstAppendInternal
					(&startCommandStr,
					 &startCommandStrSize,
					 &startCommandStrSizeMax);
				      startCommandStr[startCommandStrSize -
						      1] = ' ';
				      execute = gotonewline = true;
				    }
				  if (isnewlinechar
				      (configMemAlloc[configMemAllocPos]))
				    {
				      gotonewline = false;
				    }
				  if (configMemAlloc[configMemAllocPos] ==
				      '\r'
				      && configMemAllocPos + 1 != configSize
				      &&
				      isnewlinechar (configMemAlloc
						     [configMemAllocPos + 1]))
				    {
				      configMemAllocPos++;
				      gotonewline = false;
				    }
				  if (execute
				      && configMemAlloc[configMemAllocPos] ==
				      '#' && (!configMemAllocPos
					      ||
					      configMemAlloc[configMemAllocPos
							     - 1] == ' '
					      ||
					      isnewlinechar (configMemAlloc
							     [configMemAllocPos
							      - 1]))
				      && 1 - (nQuotes & 1))
				    {
				      execute = false;
				    }
				  if ((execute
				       && configMemAlloc[configMemAllocPos] ==
				       '\"')
				      || configMemAlloc[configMemAllocPos] ==
				      '\'')
				    {
				      if (1 - (nQuotes & 1))
					{
					  quoteType =
					    configMemAlloc[configMemAllocPos];
					}
				      nQuotes +=
					quoteType ==
					configMemAlloc[configMemAllocPos];
				    }
				  if (gotonewline & execute)
				    {
				      startCommandStrSize++;
				      setupDstAppendInternal
					(&startCommandStr,
					 &startCommandStrSize,
					 &startCommandStrSizeMax);
				      startCommandStr[startCommandStrSize -
						      1] =
					configMemAlloc[configMemAllocPos];
				    }
				}
			    }
			  startCommandStrSize++;
			  setupDstAppendInternal (&startCommandStr,
						  &startCommandStrSize,
						  &startCommandStrSizeMax);
			  startCommandStr[startCommandStrSize - 1] = 0;
			  sargv[2] = startCommandStr;
			  if (*sargv)
			    {
			      waitid (P_PID, spid, &ssiginfo, WEXITED);
			    }
			  if (sargv[2])
			    {
			      *sargv = shellLocStr;
			    }
			  if (*sargv
			      && posix_spawnp (&spid, *sargv, NULL, NULL,
					       sargv, envp) != 0)
			    {
			      *sargv = 0;
			    }
			  startCommandStrSize = startCommandStrPos;
			  execute = true;
			  gotonewline = false;
			}
		    }
		  else
		    {
		      appendInternalSrcConst (&newNameStr, &newNameStrSize,
					      &defaulticLoc,
					      &defaulticLocSize,
					      &newNameStrSizeMax);
		      appendInternalSrcConst (&newNameStr, &newNameStrSize,
					      &runshLocStr, &runshLocStrSize,
					      &newNameStrSizeMax);
		      newNameStrSize++;
		      setupDstAppendInternal (&newNameStr, &newNameStrSize,
					      &newNameStrSizeMax);
		      newNameStr[newNameStrSize - 1] = 0;
		      runFile =
			open (newNameStr, O_BINARY | O_RDWR | O_APPEND,
			      S_IRUSR | S_IWUSR);
		      if (runFile == -1)
			{
			  newNameStrSize -=
			    runshLocStrSize - runshDirLocStrSize + 1;
			  newNameStr[newNameStrSize - 1] = 0;
			  mkdim (newNameStr, envp);
			  newNameStr[newNameStrSize - 1] =
			    runshLocStr[runshDirLocStrSize - 1];
			  newNameStrSize -= runshDirLocStrSize;
			  runFile =
			    open (newNameStr,
				  O_BINARY | O_RDWR | O_CREAT | O_APPEND,
				  S_IRUSR | S_IWUSR);
			  write (runFile, runshStr, runshStrSize);
			  for (k = j = 0;
			       j !=
			       sizeof (configStrsNums) /
			       sizeof (*configStrsNums); j++)
			    {
			      appendInternalSrcConst (&newNameStr,
						      &newNameStrSize,
						      &configStrsDir[j],
						      &configStrsDirLen[j],
						      &newNameStrSizeMax);
			      newNameStrSize++;
			      setupDstAppendInternal (&newNameStr,
						      &newNameStrSize,
						      &newNameStrSizeMax);
			      newNameStr[newNameStrSize - 1] = 0;
			      mkdim (newNameStr, envp);
			      newNameStrSize--;
			      appendInternalSrcConst (&newNameStr,
						      &newNameStrSize,
						      &directorySeparator,
						      &directorySeparatorSize,
						      &newNameStrSizeMax);
			      for (l = configStrsNums[j]; l; l--)
				{
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &configStrsLoc[k],
							  &configStrsLocLen
							  [k],
							  &newNameStrSizeMax);
				  newNameStrSize++;
				  setupDstAppendInternal (&newNameStr,
							  &newNameStrSize,
							  &newNameStrSizeMax);
				  newNameStr[newNameStrSize - 1] = 0;
				  fd =
				    open (newNameStr,
					  O_BINARY | O_RDWR | O_APPEND,
					  S_IRUSR | S_IWUSR);
				  execute = fd == -1;
				  if (execute)
				    {
				      fd =
					open (newNameStr,
					      O_BINARY | O_RDWR | O_CREAT |
					      O_APPEND, S_IRUSR | S_IWUSR);
				      execute = fd != -1;
				    }
				  else
				    {
				      fstat (fd, &st);
				      execute = st.st_size == 0;
				    }
				  if (execute)
				    {
				      write (fd, configStrs[k],
					     configStrsLen[k]);
				      close (fd);
				    }
				  newNameStrSize -= configStrsLocLen[k] + 1;
				  k++;
				}
			      newNameStrSize -=
				configStrsDirLen[j] + directorySeparatorSize;
			    }
			  newNameStrSize += runshLocStrSize + 1;
			}
		      newNameStrSize -=
			defaulticLocSize + runshLocStrSize + 1;
		      if (runFile != -1)
			{
			  fstat (runFile, &st);
			  runFileSize = st.st_size;
			  runFileMemAlloc =
			    mmap (0, runFileSize, PROT_READ, MAP_SHARED,
				  runFile, 0);
			  madvise (runFileMemAlloc, runFileSize,
				   MADV_WILLNEED);
			  execute = false;
			  for (j = 0; j != runFileSize; j++)
			    {
			      if (runFileMemAlloc[j] == '\r')
				{
				  j += j + 1 != runFileSize;
				}
			      if (!gotonewline)
				{
				  runFileLine++;
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &defaulticLoc,
							  &defaulticLocSize,
							  &newNameStrSizeMax);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &runconfigLocStr,
							  &runconfigLocStrSize,
							  &newNameStrSizeMax);
				  lPos1 = runFileLine;
				  for (lSize1 = 0; lPos1; lSize1++)
				    {
				      newNameStrSize++;
				      setupDstAppendInternal (&newNameStr,
							      &newNameStrSize,
							      &newNameStrSizeMax);
				      newNameStr[newNameStrSize - 1] =
					'0' + lPos1 % 10;
				      lPos1 /= 10;
				    }
				  for (; lPos1 != lSize1 >> 1; lPos1++)
				    {
				      k =
					newNameStr[newNameStrSize + lPos1 -
						   lSize1];
				      newNameStr[newNameStrSize + lPos1 -
						 lSize1] =
					newNameStr[newNameStrSize - 1 -
						   lPos1];
				      newNameStr[newNameStrSize - lPos1 - 1] =
					k;
				    }
				  newNameStrSize++;
				  setupDstAppendInternal (&newNameStr,
							  &newNameStrSize,
							  &newNameStrSizeMax);
				  newNameStr[newNameStrSize - 1] = 0;
				  runconfig =
				    open (newNameStr,
					  O_BINARY | O_RDWR | O_APPEND,
					  S_IRUSR | S_IWUSR);
				  runconfigMemAllocPos = 0;
				  fstat (runconfig, &st);
				  runconfigSize = st.st_size;
				  runconfigMemAlloc =
				    mmap (0, runconfigSize, PROT_READ,
					  MAP_SHARED, runconfig, 0);
				  nQuotes = 0;
				  runFileMemAllocPos = j;
				  execute = j != runFileSize;
				  while (!nQuotes)
				    {
				      for (;
					   j != runFileSize
					   && runFileMemAlloc[j] == ' '; j++);
				      for (;
					   j != runFileSize
					   && runFileMemAlloc[j] == '\"'; j++)
					{
					  nQuotes++;
					}
				      execute = j != runFileSize;
				      execute = execute
					&& runFileMemAlloc[j] == 'c';
				      j += execute;
				      if (execute)
					{
					  for (;
					       j != runFileSize
					       && runFileMemAlloc[j] == '\"';
					       j++)
					    {
					      nQuotes++;
					    }
					}
				      execute = execute && j != runFileSize;
				      execute = execute
					&& runFileMemAlloc[j] == 'd';
				      if (!nQuotes)
					{
					  if (runFileMemAlloc[j] == '=')
					    {
					      break;
					    }
					  else
					    {
					      for (;
						   j != runFileSize
						   && runFileMemAlloc[j] !=
						   '='
						   && runFileMemAlloc[j] !=
						   ' '
						   && runFileMemAlloc[j] !=
						   '\"'
						   &&
						   !isnewlinechar
						   (runFileMemAlloc[j]); j++);
					      if (j == runFileSize
						  || runFileMemAlloc[j] == ' '
						  || runFileMemAlloc[j] ==
						  '\"'
						  ||
						  isnewlinechar
						  (runFileMemAlloc[j]))
						{
						  break;
						}
					      if (runFileMemAlloc[j] == '=')
						{
						  for (;
						       j != runFileSize
						       && runFileMemAlloc[j]
						       != ' '
						       &&
						       !isnewlinechar
						       (runFileMemAlloc[j]);
						       j++)
						    {
						      if (runFileMemAlloc[j]
							  == '\"')
							{
							  for (;
							       j !=
							       runFileSize
							       &&
							       runFileMemAlloc
							       [j] != '\"';
							       j++);
							}
						    }
						  nQuotes = j == runFileSize;
						}
					    }
					}
				    }
				  j += execute && j != runFileSize
				    && runFileMemAlloc[j] == 'd';
				  if (execute)
				    {
				      for (;
					   j != runFileSize
					   && runFileMemAlloc[j] == '\"'; j++)
					{
					  nQuotes++;
					}
				    }
				  execute = execute && 1 - (nQuotes & 1);
				  execute = execute && j != runFileSize;
				  if (execute)
				    {
				      for (;
					   j != runFileSize
					   && runFileMemAlloc[j] == ' '; j++);
				    }
				  execute = 1 - execute;
				  execute = execute && j != runFileSize;
				  if (execute)
				    {
				      j = runFileMemAllocPos;
				      appendInternalSrcConst
					(&startCommandStr,
					 &startCommandStrSize,
					 &commandSucessStr,
					 &commandSucessStrSize,
					 &startCommandStrSizeMax);
				    }
				  else
				    {
				      while (j != runFileSize
					     &&
					     !isnewlinechar (runFileMemAlloc
							     [j])
					     && !(runFileMemAlloc[j] == '\r'
						  && j + 1 != runFileSize
						  &&
						  isnewlinechar
						  (runFileMemAlloc[j + 1])))
					{
					  for (;
					       j != runFileSize
					       && (runFileMemAlloc[j] == '/'
						   || runFileMemAlloc[j] ==
						   '\"'); j++)
					    {
					      nQuotes +=
						runFileMemAlloc[j] == '\"';
					    }
					  for (;
					       j != runFileSize
					       && (runFileMemAlloc[j] == '\\'
						   || runFileMemAlloc[j] ==
						   '\"'); j++)
					    {
					      nQuotes +=
						runFileMemAlloc[j] == '\"';
					    }
					  if (1 - (nQuotes & 1)
					      && j != runFileSize
					      && runFileMemAlloc[j] == ' ')
					    {
					      break;
					    }
					  if (runFileMemAlloc[j] == '.')
					    {
					      if (j + 1 != runFileSize
						  && runFileMemAlloc[j + 1] ==
						  '.')
						{
						  if (j + 2 != runFileSize
						      &&
						      (runFileMemAlloc[j + 2]
						       == '/'
						       || runFileMemAlloc[j +
									  2]
						       == '\\'
						       ||
						       isnewlinechar
						       (runFileMemAlloc
							[j + 2])
						       ||
						       (runFileMemAlloc[j + 2]
							== '\r'
							&& j + 3 !=
							runFileSize
							&&
							isnewlinechar
							(runFileMemAlloc
							 [j + 3]))))
						    {
						      for (;
							   startCommandStrSize
							   != cdStrSize
							   &&
							   startCommandStr
							   [startCommandStrSize]
							   != '/'
							   &&
							   startCommandStr
							   [startCommandStrSize]
							   != '\\';
							   startCommandStrSize--);
						      execute = true;
						    }
						}
					      else if (j + 1 != runFileSize
						       &&
						       (runFileMemAlloc[j + 1]
							== '/'
							|| runFileMemAlloc[j +
									   1]
							== '\\'
							||
							isnewlinechar
							(runFileMemAlloc
							 [j + 1])
							||
							(runFileMemAlloc
							 [j + 1] == '\r'
							 && j + 2 !=
							 runFileSize
							 &&
							 isnewlinechar
							 (runFileMemAlloc
							  [j + 2]))))
						{
						  execute = true;
						}
					    }
					  if (execute)
					    {
					      for (;
						   j != runFileSize
						   && runFileMemAlloc[j] ==
						   '.'; j++);
					    }
					  else
					    {
					      appendInternalSrcConst
						(&startCommandStr,
						 &startCommandStrSize,
						 &directorySeparator,
						 &directorySeparatorSize,
						 &startCommandStrSizeMax);
					      for (;
						   j != runFileSize
						   && !(1 - (nQuotes & 1)
							&& runFileMemAlloc[j]
							== ' ')
						   && runFileMemAlloc[j] !=
						   '/'
						   && runFileMemAlloc[j] !=
						   '\\'
						   &&
						   !isnewlinechar
						   (runFileMemAlloc[j])
						   && !(runFileMemAlloc[j] ==
							'\r'
							&& j + 1 !=
							runFileSize
							&&
							isnewlinechar
							(runFileMemAlloc
							 [j + 1])); j++)
						{
						  if (runFileMemAlloc[j] ==
						      '\"')
						    {
						      nQuotes++;
						    }
						  else
						    {
						      startCommandStrSize++;
						      setupDstAppendInternal
							(&startCommandStr,
							 &startCommandStrSize,
							 &startCommandStrSizeMax);
						      startCommandStr
							[startCommandStrSize -
							 1] =
							runFileMemAlloc[j];
						    }
						}
					    }
					  execute = false;
					}
				      startCommandStrPos =
					startCommandStrSize;
				    }
				  newNameStrSize -=
				    runconfigLocStrSize + lSize1 + 1;
				  configMemAlloc = runconfigMemAlloc;
				  if (runconfigMemAlloc != MAP_FAILED)
				    {
				      madvise (runconfigMemAlloc,
					       runconfigSize, MADV_WILLNEED);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &configLocStr,
							      &configLocStrSize,
							      &newNameStrSizeMax);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &directorySeparator,
							      &directorySeparatorSize,
							      &newNameStrSizeMax);
				      appendInternal (&newNameStr,
						      &newNameStrSize,
						      &runconfigMemAlloc,
						      &runconfigSize,
						      &newNameStrSizeMax);
				      newNameStrSize++;
				      setupDstAppendInternal (&newNameStr,
							      &newNameStrSize,
							      &newNameStrSizeMax);
				      newNameStr[newNameStrSize - 1] = 0;
				      newNameStrSize -=
					configLocStrSize +
					directorySeparatorSize +
					runconfigSize + 1;
				      config =
					open (newNameStr,
					      O_BINARY | O_RDWR | O_APPEND,
					      S_IRUSR | S_IWUSR);
				      configMemAllocPos = 0;
				      fstat (config, &st);
				      configSize = st.st_size;
				      configMemAlloc =
					mmap (0, configSize, PROT_READ,
					      MAP_SHARED, config, 0);
				      if (configMemAlloc != MAP_FAILED)
					{
					  madvise (configMemAlloc, configSize,
						   MADV_WILLNEED);
					}
				    }
				  newNameStrSize -= defaulticLocSize;
				  gotonewline = true;
				}
			      if (isnewlinechar (runFileMemAlloc[j]))
				{
				  gotonewline = false;
				}
			      if (execute && gotonewline)
				{
				  startCommandStrSize++;
				  setupDstAppendInternal (&startCommandStr,
							  &startCommandStrSize,
							  &startCommandStrSizeMax);
				  startCommandStr[startCommandStrSize - 1] =
				    runFileMemAlloc[j];
				}
			      else if (execute)
				{
				  if (configMemAlloc != MAP_FAILED)
				    {
				      for (quoteType = nQuotes =
					   configMemAllocPos = 0;
					   configMemAllocPos != configSize;
					   configMemAllocPos++)
					{
					  if (!gotonewline)
					    {
					      startCommandStrSize++;
					      setupDstAppendInternal
						(&startCommandStr,
						 &startCommandStrSize,
						 &startCommandStrSizeMax);
					      startCommandStr
						[startCommandStrSize - 1] =
						' ';
					      execute = gotonewline = true;
					    }
					  if (isnewlinechar
					      (configMemAlloc
					       [configMemAllocPos]))
					    {
					      gotonewline = false;
					    }
					  if (configMemAlloc
					      [configMemAllocPos] == '\r'
					      && configMemAllocPos + 1 !=
					      configSize
					      &&
					      isnewlinechar (configMemAlloc
							     [configMemAllocPos
							      + 1]))
					    {
					      configMemAllocPos++;
					      gotonewline = false;
					    }
					  if (execute
					      &&
					      configMemAlloc
					      [configMemAllocPos] == '#'
					      && (!configMemAllocPos
						  ||
						  configMemAlloc
						  [configMemAllocPos - 1] ==
						  ' '
						  ||
						  isnewlinechar
						  (configMemAlloc
						   [configMemAllocPos - 1]))
					      && 1 - (nQuotes & 1))
					    {
					      execute = false;
					    }
					  if ((execute
					       &&
					       configMemAlloc
					       [configMemAllocPos] == '\"')
					      ||
					      configMemAlloc
					      [configMemAllocPos] == '\'')
					    {
					      if (1 - (nQuotes & 1))
						{
						  quoteType =
						    configMemAlloc
						    [configMemAllocPos];
						}
					      nQuotes +=
						quoteType ==
						configMemAlloc
						[configMemAllocPos];
					    }
					  if (gotonewline & execute)
					    {
					      startCommandStrSize++;
					      setupDstAppendInternal
						(&startCommandStr,
						 &startCommandStrSize,
						 &startCommandStrSizeMax);
					      startCommandStr
						[startCommandStrSize - 1] =
						configMemAlloc
						[configMemAllocPos];
					    }
					}
				    }
				  if (runconfigMemAlloc != MAP_FAILED)
				    {
				      madvise (runconfigMemAlloc,
					       runconfigSize, MADV_WILLNEED);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &additionalicLoc,
							      &additionalicLocSize,
							      &newNameStrSizeMax);
				      appendInternal (&newNameStr,
						      &newNameStrSize,
						      &packageNameStrs[i],
						      &packageNameStrSizes[i],
						      &newNameStrSizeMax);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &directorySeparator,
							      &directorySeparatorSize,
							      &newNameStrSizeMax);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &configLocStr,
							      &configLocStrSize,
							      &newNameStrSizeMax);
				      appendInternalSrcConst (&newNameStr,
							      &newNameStrSize,
							      &directorySeparator,
							      &directorySeparatorSize,
							      &newNameStrSizeMax);
				      appendInternal (&newNameStr,
						      &newNameStrSize,
						      &runconfigMemAlloc,
						      &runconfigSize,
						      &newNameStrSizeMax);
				      newNameStrSize++;
				      setupDstAppendInternal (&newNameStr,
							      &newNameStrSize,
							      &newNameStrSizeMax);
				      newNameStr[newNameStrSize - 1] = 0;
				      newNameStrSize -=
					additionalicLocSize +
					packageNameStrSizes[i] +
					directorySeparatorSize +
					configLocStrSize +
					directorySeparatorSize +
					runconfigSize + 1;
				      munmap (runconfigMemAlloc,
					      runconfigSize);
				      config =
					open (newNameStr,
					      O_BINARY | O_RDWR | O_APPEND,
					      S_IRUSR | S_IWUSR);
				      configMemAllocPos = 0;
				      fstat (config, &st);
				      configSize = st.st_size;
				      configMemAlloc =
					mmap (0, configSize, PROT_READ,
					      MAP_SHARED, config, 0);
				      if (configMemAlloc != MAP_FAILED)
					{
					  madvise (configMemAlloc, configSize,
						   MADV_WILLNEED);
					}
				    }
				  gotonewline = false;
				  if (configMemAlloc != MAP_FAILED)
				    {
				      for (quoteType = nQuotes =
					   configMemAllocPos = 0;
					   configMemAllocPos != configSize;
					   configMemAllocPos++)
					{
					  if (!gotonewline)
					    {
					      startCommandStrSize++;
					      setupDstAppendInternal
						(&startCommandStr,
						 &startCommandStrSize,
						 &startCommandStrSizeMax);
					      startCommandStr
						[startCommandStrSize - 1] =
						' ';
					      execute = gotonewline = true;
					    }
					  if (isnewlinechar
					      (configMemAlloc
					       [configMemAllocPos]))
					    {
					      gotonewline = false;
					    }
					  if (configMemAlloc
					      [configMemAllocPos] == '\r'
					      && configMemAllocPos + 1 !=
					      configSize
					      &&
					      isnewlinechar (configMemAlloc
							     [configMemAllocPos
							      + 1]))
					    {
					      configMemAllocPos++;
					      gotonewline = false;
					    }
					  if (execute
					      &&
					      configMemAlloc
					      [configMemAllocPos] == '#'
					      && (!configMemAllocPos
						  ||
						  configMemAlloc
						  [configMemAllocPos - 1] ==
						  ' '
						  ||
						  isnewlinechar
						  (configMemAlloc
						   [configMemAllocPos - 1]))
					      && 1 - (nQuotes & 1))
					    {
					      execute = false;
					    }
					  if ((execute
					       &&
					       configMemAlloc
					       [configMemAllocPos] == '\"')
					      ||
					      configMemAlloc
					      [configMemAllocPos] == '\'')
					    {
					      if (1 - (nQuotes & 1))
						{
						  quoteType =
						    configMemAlloc
						    [configMemAllocPos];
						}
					      nQuotes +=
						quoteType ==
						configMemAlloc
						[configMemAllocPos];
					    }
					  if (gotonewline & execute)
					    {
					      startCommandStrSize++;
					      setupDstAppendInternal
						(&startCommandStr,
						 &startCommandStrSize,
						 &startCommandStrSizeMax);
					      startCommandStr
						[startCommandStrSize - 1] =
						configMemAlloc
						[configMemAllocPos];
					    }
					}
				    }
				  startCommandStrSize++;
				  setupDstAppendInternal (&startCommandStr,
							  &startCommandStrSize,
							  &startCommandStrSizeMax);
				  startCommandStr[startCommandStrSize - 1] =
				    0;
				  sargv[2] = startCommandStr;
				  if (*sargv)
				    {
				      waitid (P_PID, spid, &ssiginfo,
					      WEXITED);
				    }
				  if (sargv[2])
				    {
				      *sargv = shellLocStr;
				    }
				  if (*sargv
				      && posix_spawnp (&spid, *sargv, NULL,
						       NULL, sargv,
						       envp) != 0)
				    {
				      *sargv = 0;
				    }
				  startCommandStrSize = startCommandStrPos;
				  execute = true;
				  gotonewline = false;
				}
			    }
			  if (execute)
			    {
			      if (configMemAlloc != MAP_FAILED)
				{
				  for (quoteType = nQuotes =
				       configMemAllocPos = 0;
				       configMemAllocPos != configSize;
				       configMemAllocPos++)
				    {
				      if (!gotonewline)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] = ' ';
					  execute = gotonewline = true;
					}
				      if (isnewlinechar
					  (configMemAlloc[configMemAllocPos]))
					{
					  gotonewline = false;
					}
				      if (configMemAlloc[configMemAllocPos] ==
					  '\r'
					  && configMemAllocPos + 1 !=
					  configSize
					  &&
					  isnewlinechar (configMemAlloc
							 [configMemAllocPos +
							  1]))
					{
					  configMemAllocPos++;
					  gotonewline = false;
					}
				      if (execute
					  && configMemAlloc[configMemAllocPos]
					  == '#' && (!configMemAllocPos
						     ||
						     configMemAlloc
						     [configMemAllocPos -
						      1] == ' '
						     ||
						     isnewlinechar
						     (configMemAlloc
						      [configMemAllocPos -
						       1]))
					  && 1 - (nQuotes & 1))
					{
					  execute = false;
					}
				      if ((execute
					   &&
					   configMemAlloc[configMemAllocPos]
					   == '\"')
					  || configMemAlloc[configMemAllocPos]
					  == '\'')
					{
					  if (1 - (nQuotes & 1))
					    {
					      quoteType =
						configMemAlloc
						[configMemAllocPos];
					    }
					  nQuotes +=
					    quoteType ==
					    configMemAlloc[configMemAllocPos];
					}
				      if (gotonewline & execute)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] =
					    configMemAlloc[configMemAllocPos];
					}
				    }
				}
			      if (runconfigMemAlloc != MAP_FAILED)
				{
				  madvise (runconfigMemAlloc, runconfigSize,
					   MADV_WILLNEED);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &additionalicLoc,
							  &additionalicLocSize,
							  &newNameStrSizeMax);
				  appendInternal (&newNameStr,
						  &newNameStrSize,
						  &packageNameStrs[i],
						  &packageNameStrSizes[i],
						  &newNameStrSizeMax);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &directorySeparator,
							  &directorySeparatorSize,
							  &newNameStrSizeMax);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &configLocStr,
							  &configLocStrSize,
							  &newNameStrSizeMax);
				  appendInternalSrcConst (&newNameStr,
							  &newNameStrSize,
							  &directorySeparator,
							  &directorySeparatorSize,
							  &newNameStrSizeMax);
				  appendInternal (&newNameStr,
						  &newNameStrSize,
						  &runconfigMemAlloc,
						  &runconfigSize,
						  &newNameStrSizeMax);
				  newNameStrSize++;
				  setupDstAppendInternal (&newNameStr,
							  &newNameStrSize,
							  &newNameStrSizeMax);
				  newNameStr[newNameStrSize - 1] = 0;
				  newNameStrSize -=
				    additionalicLocSize +
				    packageNameStrSizes[i] +
				    directorySeparatorSize +
				    configLocStrSize +
				    directorySeparatorSize + runconfigSize +
				    1;
				  munmap (runconfigMemAlloc, runconfigSize);
				  config =
				    open (newNameStr,
					  O_BINARY | O_RDWR | O_APPEND,
					  S_IRUSR | S_IWUSR);
				  configMemAllocPos = 0;
				  fstat (config, &st);
				  configSize = st.st_size;
				  configMemAlloc =
				    mmap (0, configSize, PROT_READ,
					  MAP_SHARED, config, 0);
				  if (configMemAlloc != MAP_FAILED)
				    {
				      madvise (configMemAlloc, configSize,
					       MADV_WILLNEED);
				    }
				}
			      gotonewline = false;
			      if (configMemAlloc != MAP_FAILED)
				{
				  for (quoteType = nQuotes =
				       configMemAllocPos = 0;
				       configMemAllocPos != configSize;
				       configMemAllocPos++)
				    {
				      if (!gotonewline)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] = ' ';
					  execute = gotonewline = true;
					}
				      if (isnewlinechar
					  (configMemAlloc[configMemAllocPos]))
					{
					  gotonewline = false;
					}
				      if (configMemAlloc[configMemAllocPos] ==
					  '\r'
					  && configMemAllocPos + 1 !=
					  configSize
					  &&
					  isnewlinechar (configMemAlloc
							 [configMemAllocPos +
							  1]))
					{
					  configMemAllocPos++;
					  gotonewline = false;
					}
				      if (execute
					  && configMemAlloc[configMemAllocPos]
					  == '#' && (!configMemAllocPos
						     ||
						     configMemAlloc
						     [configMemAllocPos -
						      1] == ' '
						     ||
						     isnewlinechar
						     (configMemAlloc
						      [configMemAllocPos -
						       1]))
					  && 1 - (nQuotes & 1))
					{
					  execute = false;
					}
				      if ((execute
					   &&
					   configMemAlloc[configMemAllocPos]
					   == '\"')
					  || configMemAlloc[configMemAllocPos]
					  == '\'')
					{
					  if (1 - (nQuotes & 1))
					    {
					      quoteType =
						configMemAlloc
						[configMemAllocPos];
					    }
					  nQuotes +=
					    quoteType ==
					    configMemAlloc[configMemAllocPos];
					}
				      if (gotonewline & execute)
					{
					  startCommandStrSize++;
					  setupDstAppendInternal
					    (&startCommandStr,
					     &startCommandStrSize,
					     &startCommandStrSizeMax);
					  startCommandStr[startCommandStrSize
							  - 1] =
					    configMemAlloc[configMemAllocPos];
					}
				    }
				}
			      startCommandStrSize++;
			      setupDstAppendInternal (&startCommandStr,
						      &startCommandStrSize,
						      &startCommandStrSizeMax);
			      startCommandStr[startCommandStrSize - 1] = 0;
			      sargv[2] = startCommandStr;
			      if (*sargv)
				{
				  waitid (P_PID, spid, &ssiginfo, WEXITED);
				}
			      if (sargv[2])
				{
				  *sargv = shellLocStr;
				}
			      if (*sargv
				  && posix_spawnp (&spid, *sargv, NULL, NULL,
						   sargv, envp) != 0)
				{
				  *sargv = 0;
				}
			      startCommandStrSize = startCommandStrPos;
			      execute = true;
			      gotonewline = false;
			    }
			}
		    }
		  startCommandStrSize = 0;
		  installDirNameSize--;
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  &installDir, &installDirNameSize,
				  &startCommandStrSizeMax);
		  installDirNameSize++;
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &archivesLoc,
					  &archivesLocSize,
					  &startCommandStrSizeMax);
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  &packageNameStrs[i],
				  &packageNameStrSizes[i],
				  &startCommandStrSizeMax);
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize,
					  &installLocStr, &installLocStrSize,
					  &startCommandStrSizeMax);
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &infoLocStr,
					  &infoLocStrSize,
					  &startCommandStrSizeMax);
		  startCommandStr[startCommandStrSize] = 0;
		  mkdim (startCommandStr, envp);
		  appendInternal (&startCommandStr, &startCommandStrSize,
				  &packageNameStrs[i],
				  &packageNameStrSizes[i],
				  &startCommandStrSizeMax);
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize,
					  &listFileExtensionStr,
					  &listFileExtensionStrSize,
					  &startCommandStrSizeMax);
		  startCommandStr[startCommandStrSize] = 0;
		  newListFile =
		    open (startCommandStr, O_BINARY | O_RDWR | O_CREAT);
		  startCommandStrSize -=
		    listFileExtensionStrSize + packageNameStrSizes[i] +
		    infoLocStrSize;
		  startCommandStr[startCommandStrSize] = 0;
		  startILFE =
		    (struct internalListFileEntry *)
		    malloc (sizeof (*startILFE));
		  startILFE->fileLoc = ".";
		  startILFE->fileLocSize = strlen (startILFE->fileLoc) + 1;
		  startILFE->directoryLoc = "/";
		  startILFE->directoryLocSize =
		    strlen (startILFE->directoryLoc);
		  prevdirectory = startILFE;
		  startILFE->prevdirectory = prevdirectory;
		  startILFE->isdirectory = 1;
		  startILFE->next =
		    (struct internalListFileEntry *)
		    malloc (sizeof (*currILFE));
		  *startILFE->next = *startILFE;
		  previDS =
		    (struct internalDirectoryScan *)
		    malloc (sizeof (*previDS));
		  previDS->iDirectory = startILFE->next;
		  previDS->prevdirectory = prevdirectory;
		  previDS->prev = 0;
		  currILFE = startILFE;
		  curriDS = previDS;
		  dirp = opendir (startCommandStr);
		  newListFileSize =
		    currILFE->fileLocSize + currILFE->directoryLocSize;
		  if (*sargv)
		    {
		      waitid (P_PID, spid, &ssiginfo, WEXITED);
		    }
		  *sargv = "test";
		  sargv[1] = "-L";
		  sargv[3] = 0;
		  if (dirp)
		    {
		      curriDS->dirp = dirp;
		      while (curriDS)
			{
			  prevdirectory = curriDS->prevdirectory;
			  previDS = curriDS->prev;
			  dirp = curriDS->dirp;
			  while ((dir = readdir (dirp)) != NULL)
			    {
			      if ((!
				   (dir->d_name[0] == '.'
				    && dir->d_name[1] == 0))
				  &&
				  (!(dir->d_name[0] == '.'
				     && dir->d_name[1] == '.'
				     && dir->d_name[2] == 0)))
				{
				  currILFE->next->next =
				    (struct internalListFileEntry *)
				    malloc (sizeof (*currILFE));
				  currILFE = currILFE->next;
				  currILFE->fileLocSize =
				    strlen (dir->d_name) + 1;
				  currILFE->fileLoc =
				    (char *) malloc (currILFE->fileLocSize *
						     sizeof (char));
				  currILFE->fileLoc[currILFE->fileLocSize -
						    1] = 0;
				  currILFE->directoryLoc =
				    curriDS->iDirectory->directoryLoc;
				  currILFE->directoryLocSize =
				    curriDS->iDirectory->directoryLocSize;
				  currILFE->prevdirectory = prevdirectory;
				  currILFE->isdirectory = 0;
				  memcpy (currILFE->fileLoc, dir->d_name,
					  currILFE->fileLocSize *
					  sizeof (char));
				  newListFileSize +=
				    currILFE->fileLocSize +
				    currILFE->directoryLocSize;
				  appendInternal (&startCommandStr,
						  &startCommandStrSize,
						  &currILFE->directoryLoc,
						  &currILFE->directoryLocSize,
						  &startCommandStrSizeMax);
				  appendInternal (&startCommandStr,
						  &startCommandStrSize,
						  &currILFE->fileLoc,
						  &currILFE->fileLocSize,
						  &startCommandStrSizeMax);
				  startCommandStr[startCommandStrSize - 1] =
				    0;
				  sargv[2] = startCommandStr;
				  if (sargv[2]
				      && !posix_spawnp (&spid, *sargv, NULL,
							NULL, sargv, envp))
				    {
				      if (waitid
					  (P_PID, spid, &ssiginfo,
					   WEXITED) != -1)
					{
					  if (ssiginfo.si_signo != SIGCHLD
					      || ssiginfo.si_code !=
					      CLD_EXITED)
					    {
					      ssiginfo.si_status = 1;
					    }
					}
				      else
					{
					  ssiginfo.si_status = 1;
					}
				    }
				  else
				    {
				      ssiginfo.si_status = 1;
				    }
				  if (ssiginfo.si_status)
				    {
				      dirp = opendir (startCommandStr);
				      if (dirp)
					{
					  currILFE->isdirectory = 1;
					  prevdirectory = currILFE;
					  previDS = curriDS;
					  curriDS =
					    (struct internalDirectoryScan *)
					    malloc (sizeof (*previDS));
					  curriDS->prevdirectory =
					    prevdirectory;
					  curriDS->iDirectory =
					    currILFE->next;
					  curriDS->iDirectory->
					    directoryLocSize =
					    currILFE->directoryLocSize +
					    currILFE->fileLocSize;
					  curriDS->iDirectory->directoryLoc =
					    (char *) malloc (curriDS->
							     iDirectory->
							     directoryLocSize);
					  memcpy (curriDS->iDirectory->
						  directoryLoc,
						  currILFE->directoryLoc,
						  currILFE->directoryLocSize);
					  memcpy (curriDS->iDirectory->
						  directoryLoc +
						  currILFE->directoryLocSize,
						  currILFE->fileLoc,
						  currILFE->fileLocSize);
					  curriDS->iDirectory->
					    directoryLoc[curriDS->iDirectory->
							 directoryLocSize -
							 1] = '/';
					  curriDS->dirp = dirp;
					  curriDS->prev = previDS;
					}
				      dirp = curriDS->dirp;
				    }
				  startCommandStrSize -=
				    currILFE->directoryLocSize;
				  startCommandStrSize -=
				    currILFE->fileLocSize;
				}
			    }
			  closedir (dirp);
			  previDS = curriDS->prev;
			  free (curriDS);
			  curriDS = previDS;
			}
		    }
		  currILFE->next = 0;
		  newListFileMemAlloc = (char *) malloc (newListFileSize);
		  newListFileMemAllocPos = 0;
		  currILFE = startILFE;
		  while (currILFE)
		    {
		      memcpy (newListFileMemAlloc + newListFileMemAllocPos,
			      currILFE->directoryLoc,
			      currILFE->directoryLocSize);
		      newListFileMemAllocPos += currILFE->directoryLocSize;
		      newListFileMemAlloc[newListFileMemAllocPos - 1] = '/';
		      memcpy (newListFileMemAlloc + newListFileMemAllocPos,
			      currILFE->fileLoc, currILFE->fileLocSize);
		      newListFileMemAllocPos += currILFE->fileLocSize;
		      newListFileMemAlloc[newListFileMemAllocPos - 1] = '\n';
		      currILFE = currILFE->next;
		    }
		  write (newListFile, newListFileMemAlloc,
			 newListFileMemAllocPos);
		  close (newListFile);
		  free (newListFileMemAlloc);
		  newNameStrSize = 0;
		  installDirNameSize -= 2;
		  appendInternal (&newNameStr, &newNameStrSize, &installDir,
				  &installDirNameSize, &newNameStrSizeMax);
		  installDirNameSize += 2;
		  currILFE = startILFE;
		  currILFE = currILFE->next;
		  while (currILFE)
		    {
		      if (currILFE->prevdirectory
			  && currILFE->prevdirectory->prevdirectory)
			{
			  appendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &currILFE->directoryLoc,
					  &currILFE->directoryLocSize,
					  &startCommandStrSizeMax);
			  appendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &currILFE->fileLoc,
					  &currILFE->fileLocSize,
					  &startCommandStrSizeMax);
			  appendInternalSrcConst (&startCommandStr,
						  &startCommandStrSize,
						  &checkIfSymbolicLinkEndStr,
						  &checkIfSymbolicLinkEndStrSize,
						  &startCommandStrSizeMax);
			  appendInternal (&newNameStr, &newNameStrSize,
					  &currILFE->directoryLoc,
					  &currILFE->directoryLocSize,
					  &newNameStrSizeMax);
			  appendInternal (&newNameStr, &newNameStrSize,
					  &currILFE->fileLoc,
					  &currILFE->fileLocSize,
					  &newNameStrSizeMax);
			  appendInternalSrcConst (&newNameStr,
						  &newNameStrSize,
						  &checkIfSymbolicLinkEndStr,
						  &checkIfSymbolicLinkEndStrSize,
						  &newNameStrSizeMax);
			  startCommandStr[startCommandStrSize - 2] = 0;
			  if (!rename (startCommandStr, newNameStr))
			    {
			      currILFE->prevdirectory = 0;
			    }
			  newNameStrSize -= currILFE->directoryLocSize;
			  newNameStrSize -= currILFE->fileLocSize;
			  newNameStrSize -= checkIfSymbolicLinkEndStrSize;
			  startCommandStrSize -= currILFE->directoryLocSize;
			  startCommandStrSize -= currILFE->fileLocSize;
			  startCommandStrSize -=
			    checkIfSymbolicLinkEndStrSize;
			}
		      else
			{
			  currILFE->prevdirectory = 0;
			}
		      currILFE = currILFE->next;
		    }
		  tmpNewNameStrSize = 0;
		  currILFE = startILFE;
		  currILFE = currILFE->next;
		  while (currILFE)
		    {
		      if (!currILFE->isdirectory)
			{
			  if (currILFE->prevdirectory
			      && currILFE->prevdirectory->prevdirectory)
			    {
			      if (!tmpNewNameStrSize)
				{
				  installDirNameSize -= 2;
				  appendInternal (&tmpNewNameStr,
						  &tmpNewNameStrSize,
						  &installDir,
						  &installDirNameSize,
						  &tmpNewNameStrSizeMax);
				  installDirNameSize += 2;
				}
			      appendInternal (&startCommandStr,
					      &startCommandStrSize,
					      &currILFE->directoryLoc,
					      &currILFE->directoryLocSize,
					      &startCommandStrSizeMax);
			      appendInternal (&startCommandStr,
					      &startCommandStrSize,
					      &currILFE->fileLoc,
					      &currILFE->fileLocSize,
					      &startCommandStrSizeMax);
			      appendInternal (&newNameStr, &newNameStrSize,
					      &currILFE->directoryLoc,
					      &currILFE->directoryLocSize,
					      &newNameStrSizeMax);
			      appendInternal (&newNameStr, &newNameStrSize,
					      &currILFE->fileLoc,
					      &currILFE->fileLocSize,
					      &newNameStrSizeMax);
			      newNameStr += installDirNameSize - 2;
			      newNameStrSize -= installDirNameSize - 2;
			      appendInternal (&tmpNewNameStr,
					      &tmpNewNameStrSize, &newNameStr,
					      &newNameStrSize,
					      &tmpNewNameStrSizeMax);
			      newNameStr -= installDirNameSize - 2;
			      newNameStrSize += installDirNameSize - 2;
			      tmpNewNameStrSize--;
			      appendInternalSrcConst (&tmpNewNameStr,
						      &tmpNewNameStrSize,
						      &tmpFileExtensionStr,
						      &tmpFileExtensionStrSize,
						      &tmpNewNameStrSizeMax);
			      tmpNewNameStrSize++;
			      setupDstAppendInternal (&tmpNewNameStr,
						      &tmpNewNameStrSize,
						      &tmpNewNameStrSizeMax);
			      tmpNewNameStr[tmpNewNameStrSize - 1] = 0;
			      if (!stat (tmpNewNameStr, &st))
				{
				  tmpNewNameStrSize -=
				    tmpFileExtensionStrSize;
				  tmpNewNumberPos = tmpNewNameStrSize;
				  tmpNewNumberPosMin = tmpNewNumberPos;
				  tmpNewNumberPosMax = tmpNewNumberPos;
				  tmpNewNameStr[tmpNewNumberPos] = '1';
				  tmpNewNameStrSize++;
				  appendInternalSrcConst (&tmpNewNameStr,
							  &tmpNewNameStrSize,
							  &tmpFileExtensionStr,
							  &tmpFileExtensionStrSize,
							  &tmpNewNameStrSizeMax);
				  tmpNewNameStrSize++;
				  setupDstAppendInternal (&tmpNewNameStr,
							  &tmpNewNameStrSize,
							  &tmpNewNameStrSizeMax);
				  tmpNewNameStr[tmpNewNameStrSize - 1] = 0;
				  while (!stat (tmpNewNameStr, &st))
				    {
				      if (tmpNewNameStr[tmpNewNumberPos] ==
					  '9')
					{
					  if (tmpNewNumberPos ==
					      tmpNewNumberPosMin)
					    {
					      tmpNewNumberPosMax++;
					      tmpNewNameStrSize =
						tmpNewNumberPosMax + 1;
					      tmpNewNameStr[tmpNewNumberPos] =
						'1';
					      for (tmpNewNumberPos++;
						   tmpNewNumberPos !=
						   tmpNewNumberPosMax;
						   tmpNewNumberPos++)
						{
						  tmpNewNameStr
						    [tmpNewNumberPos] = '0';
						}
					      tmpNewNameStr
						[tmpNewNumberPosMax] = '0';
					      tmpNewNumberPos =
						tmpNewNumberPosMax;
					      appendInternalSrcConst
						(&tmpNewNameStr,
						 &tmpNewNameStrSize,
						 &tmpFileExtensionStr,
						 &tmpFileExtensionStrSize,
						 &tmpNewNameStrSizeMax);
					      tmpNewNameStrSize++;
					      setupDstAppendInternal
						(&tmpNewNameStr,
						 &tmpNewNameStrSize,
						 &tmpNewNameStrSizeMax);
					      tmpNewNameStr[tmpNewNameStrSize
							    - 1] = 0;
					    }
					  else
					    {
					      tmpNewNameStr[tmpNewNumberPos] =
						'0';
					      tmpNewNumberPos--;
					    }
					}
				      else
					{
					  tmpNewNameStr[tmpNewNumberPos]++;
					  if (tmpNewNumberPos !=
					      tmpNewNumberPosMax)
					    {
					      tmpNewNumberPos =
						tmpNewNumberPosMax;
					    }
					}
				    }
				}
			      rename (newNameStr, tmpNewNameStr);
			      if (!rename (startCommandStr, newNameStr))
				{
				  if (remove (tmpNewNameStr))
				    {
				      chmod (tmpNewNameStr, S_IWUSR);
				      remove (tmpNewNameStr);
				    }
				  currILFE->prevdirectory = 0;
				}
			      tmpNewNameStrSize = installDirNameSize - 2;
			      newNameStrSize -= currILFE->directoryLocSize;
			      newNameStrSize -= currILFE->fileLocSize;
			      startCommandStrSize -=
				currILFE->directoryLocSize;
			      startCommandStrSize -= currILFE->fileLocSize;
			    }
			  else
			    {
			      currILFE->prevdirectory = 0;
			    }
			}
		      currILFE = currILFE->next;
		    }
		  startCommandStrSize -= installLocStrSize;
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &gitLocStr,
					  &gitLocStrSize,
					  &startCommandStrSizeMax);
		  appendInternalSrcConst (&startCommandStr,
					  &startCommandStrSize, &HEADLocStr,
					  &HEADLocStrSize,
					  &startCommandStrSizeMax);
		  startCommandStrSize++;
		  setupDstAppendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &startCommandStrSizeMax);
		  startCommandStr[startCommandStrSize - 1] = 0;
		  headFile = open (startCommandStr, O_BINARY | O_RDWR);
		  fstat (headFile, &st);
		  headFileSize = st.st_size;
		  headFileMemMap =
		    mmap (0, headFileSize, PROT_READ, MAP_SHARED, headFile,
			  0);
		  madvise (headFileMemMap, headFileSize, MADV_WILLNEED);
		  startCommandStrSize--;
		  for (j = 0; j != headFileSize; j++)
		    {
		      if (headFileMemMap[j] == 'r'
			  && headFileMemMap[j + 1] == 'e'
			  && headFileMemMap[j + 2] == 'f'
			  && headFileMemMap[j + 3] == ':'
			  && headFileMemMap[j + 4] == ' ')
			{
			  j += 5;
			  headFileMemMap += j;
			  startCommandStrSize -= HEADLocStrSize;
			  for (k = 0;
			       headFileMemMap[k] != '\n' && k != headFileSize;
			       k++);
			  appendInternal (&startCommandStr,
					  &startCommandStrSize,
					  &headFileMemMap, &k,
					  &startCommandStrSizeMax);
			  startCommandStrSize++;
			  setupDstAppendInternal (&startCommandStr,
						  &startCommandStrSize,
						  &startCommandStrSizeMax);
			  startCommandStr[startCommandStrSize - 1] = 0;
			  headFileMemMap -= j;
			  j = headFileSize - 1;
			}
		    }
		  munmap (headFileMemMap, headFileSize);
		  close (headFile);
		  headFile = open (startCommandStr, O_BINARY | O_RDWR);
		  fstat (headFile, &st);
		  headFileSize = st.st_size;
		  headFileMemMap =
		    mmap (0, headFileSize, PROT_READ, MAP_SHARED, headFile,
			  0);
		  madvise (headFileMemMap, headFileSize, MADV_WILLNEED);
		  for (j = 0; headFileMemMap[j] != '\n' && j != headFileSize;
		       j++);
		  gotonewline = false;
		  statusFileMemAllocPos = 0;
		  for (k = 0; k != statusFileSize; k++)
		    {
		      if (statusFileMemAllocPos)
			{
			  if (k - statusFileMemAllocPos ==
			      packageNameStrSizes[i])
			    {
			      if (statusFileMemAlloc[k] == '\n')
				{
				  k = statusFileSize - 1;
				}
			      else
				{
				  statusFileMemAllocPos = 0;
				}
			    }
			  else
			    {
			      if (statusFileMemAlloc[k] !=
				  packageNameStrs[i][k -
						     statusFileMemAllocPos])
				{
				  statusFileMemAllocPos = 0;
				}
			    }
			}
		      if (!gotonewline)
			{
			  if (!strncmp
			      (statusFileMemAlloc + k, "Package: ", 9))
			    {
			      k += 9;
			      statusFileMemAllocPos = k;
			    }
			}
		      gotonewline = statusFileMemAlloc[k] != '\n';
		    }
		  gotonewline = false;
		  commitHash = 0;
		  commitHashSize = 0;
		  for (k = statusFileMemAllocPos; k != statusFileSize; k++)
		    {
		      if (!gotonewline)
			{
			  if (!strncmp
			      (statusFileMemAlloc + k, "CommitHash: ", 12))
			    {
			      k += 12;
			      commitHash = statusFileMemAlloc + k;
			      statusFileMemAllocPos = k;
			    }
			  else
			    if (!strncmp
				(statusFileMemAlloc + k, "Package: ", 9))
			    {
			      k = statusFileSize - 1;
			    }
			}
		      gotonewline = statusFileMemAlloc[k] != '\n';
		      if (commitHash)
			{
			  commitHashSize += gotonewline;
			  if (!gotonewline)
			    {
			      k = statusFileSize - 1;
			    }
			}
		    }
		  if (j > commitHashSize)
		    {
		      memcpy (commitHash, headFileMemMap, commitHashSize);
		      k = j - commitHashSize;
		      statusFileAddLoc[statusFileAddLocPos]
			[statusFileAddLocSize] =
			statusFileMemAllocPos + commitHashSize;
		      statusFileAddSize[statusFileAddLocPos]
			[statusFileAddLocSize] = k;
		      newstatusFileSize += k;
		      statusFileAddLocSize++;
		      selectedLength2++;
		      statusFileAddStrSize += k;
		      if (statusFileAddStrSize <= 1 << statusFileAddStrPos)
			{
			  memcpy (statusFileAddStr[statusFileAddStrPos] +
				  statusFileAddStrSize - k,
				  headFileMemMap + commitHashSize, k);
			  if (statusFileAddStrSize ==
			      1 << statusFileAddStrPos)
			    {
			      statusFileAddStrSize = 0;
			      statusFileAddStrPos++;
			      statusFileAddStr[statusFileAddStrPos] =
				(char *) malloc (sizeof (char) <<
						 statusFileAddStrPos);
			    }
			}
		      else
			{
			  memcpy (statusFileAddStr[statusFileAddStrPos] +
				  statusFileAddStrSize - k,
				  headFileMemMap + commitHashSize,
				  (1 << statusFileAddStrPos) -
				  statusFileAddStrSize + k);
			  statusFileAddStrSize =
			    (1 << statusFileAddStrPos) -
			    statusFileAddStrSize + k;
			  k -= statusFileAddStrSize;
			  statusFileAddStrSize += commitHashSize;
			  while (k != 0)
			    {
			      statusFileAddStrPos++;
			      statusFileAddStr[statusFileAddStrPos] =
				(char *) malloc (sizeof (char) <<
						 statusFileAddStrPos);
			      if (k > 1 << statusFileAddStrPos)
				{
				  memcpy (statusFileAddStr
					  [statusFileAddStrPos],
					  headFileMemMap +
					  statusFileAddStrSize,
					  1 << statusFileAddStrPos);
				}
			      else
				{
				  memcpy (statusFileAddStr
					  [statusFileAddStrPos],
					  headFileMemMap +
					  statusFileAddStrSize, k);
				  statusFileAddStrSize =
				    k - (1 << statusFileAddStrPos);
				  k = 1 << statusFileAddStrPos;
				}
			      statusFileAddStrSize +=
				1 << statusFileAddStrPos;
			      k -= 1 << statusFileAddStrPos;
			    }
			  if (statusFileAddStrSize ==
			      1 << statusFileAddStrPos)
			    {
			      statusFileAddStrPos++;
			      statusFileAddStr[statusFileAddStrPos] =
				(char *) malloc (sizeof (char) <<
						 statusFileAddStrPos);
			      statusFileAddStrSize = 0;
			    }
			}
		      if (statusFileAddLocSize == 1 << statusFileAddLocPos)
			{
			  statusFileAddLocSize = 0;
			  statusFileAddLocPos++;
			  statusFileAddLoc[statusFileAddLocPos] =
			    (size_t *) malloc (sizeof (size_t) <<
					       statusFileAddLocPos);
			  statusFileAddSize[statusFileAddLocPos] =
			    (size_t *) malloc (sizeof (size_t) <<
					       statusFileAddLocPos);
			  statusFileAddStrLocPos++;
			}
		    }
		  else
		    {
		      memcpy (commitHash, headFileMemMap, j);
		      if (j != commitHashSize)
			{
			  statusFileRanges[statusFileRangesPos]
			    [statusFileRangesSize] =
			    statusFileMemAllocPos + j;
			  statusFileRanges[statusFileRangesPos]
			    [statusFileRangesSize + 1] =
			    statusFileMemAllocPos + commitHashSize;
			  statusFileRangesSize += 2;
			  selectedLength1++;
			  newstatusFileSize -=
			    statusFileMemAllocPos + commitHashSize -
			    statusFileMemAllocPos - j;
			  if (statusFileRangesSize ==
			      1 << (statusFileRangesPos + 1))
			    {
			      statusFileRangesSize = 0;
			      statusFileRangesPos++;
			      statusFileRanges[statusFileRangesPos] =
				(size_t *) malloc (sizeof (size_t) *
						   2 << statusFileRangesPos);
			    }
			}
		    }
		  munmap (headFileMemMap, headFileSize);
		  close (headFile);
		}
	      statusFileRanges[statusFileRangesPos][statusFileRangesSize] =
		statusFileSize;
	      statusFileRanges[statusFileRangesPos][statusFileRangesSize +
						    1] = statusFileSize;
	      if (newstatusFileSize > statusFileSize)
		{
		  munmap (statusFileMemAlloc, statusFileSize);
		  statusFileRanges[statusFileRangesPos][statusFileRangesSize +
							1] =
		    newstatusFileSize;
		  statusFileRangesSize += 2;
		  selectedLength1++;
		  if (statusFileRangesSize == 1 << (statusFileRangesPos + 1))
		    {
		      statusFileRangesSize = 0;
		      statusFileRangesPos++;
		    }
		  statusFileSize = newstatusFileSize;
		  ftruncate (statusFile, statusFileSize);
		  statusFileMemAlloc =
		    mmap (0, statusFileSize, PROT_READ | PROT_WRITE,
			  MAP_SHARED, statusFile, 0);
		}
	      madvise (statusFileMemAlloc, statusFileSize, MADV_WILLNEED);
	      statusFileAddLocPosMax = statusFileAddLocPos;
	      statusFileAddLocSizeMax = statusFileAddLocSize;
	      statusFileRangesPosMax = statusFileRangesPos;
	      statusFileRangesSizeMax = statusFileRangesSize;
	      if (!statusFileRequestSorted)
		{
		  statusFileAddLocPos = statusFileAddLocSize =
		    statusFileRangesPos = statusFileRangesSize = 0;
		  statusFileAddStRSize =
		    (size_t *) malloc (statusFileAddStrSizeMax *
				       sizeof (*statusFileAddStRSize));
		  for (i = 0; i != statusFileAddLocPosMax; i++)
		    {
		      offset[i] = (size_t *) malloc (sizeof (size_t) << i);
		      for (j = 0; j != 1 << i; j++)
			{
			  offset[i][j] =
			    (i << (sizeof (offset[i][j]) * 8 - 6)) + j;
			}
		    }
		  if (statusFileAddLocSizeMax)
		    {
		      offset[i] = (size_t *) malloc (sizeof (size_t) << i);
		      for (j = 0; j != 1 << i; j++)
			{
			  offset[i][j] =
			    (i << (sizeof (offset[i][j]) * 8 - 6)) + j;
			}
		    }
		  radixes1 = (size_t *) malloc (sizeof (size_t) * 256 * 8);
		  radixes2 = (size_t *) malloc (sizeof (size_t) * 256 * 8);
		  radixoffPos1 = (size_t *) malloc (sizeof (size_t) * 256);
		  radixoffSize1 = (size_t *) malloc (sizeof (size_t) * 256);
		  radixoffPos2 = (size_t *) malloc (sizeof (size_t) * 256);
		  radixoffSize2 = (size_t *) malloc (sizeof (size_t) * 256);
		  radix1 = radixes1;
		  radix2 = radixes2;
		  statusFileAddLocPos = statusFileAddLocSize =
		    statusFileRangesPos = statusFileRangesSize = 0;
		  i = 0;
		  j = 0;
		  k = 0;
		  l = 0;
		  if (!selectedLength1)
		    {
		      k = sizeof (i);
		    }
		  if (!selectedLength2)
		    {
		      l = sizeof (j);
		    }
		  while (k != sizeof (i) && l != sizeof (j))
		    {
		      for (m = 0; m != 256; m++)
			{
			  radix1[m] = 0;
			  radix2[m] = 0;
			}
		      s1 = 64 - 8 - (k * 8);
		      s2 = 64 - 8 - (l * 8);
		      lPos1 = statusFileRangesPos;
		      lSize1 = statusFileRangesSize;
		      lPos2 = statusFileAddLocPos;
		      lSize2 = statusFileAddLocSize;
		      m = 0;
		      m -= selectedLength1 > selectedLength2;
		      for (m =
			   (selectedLength1 & (0 - m - 1)) +
			   (selectedLength2 & m); m; m--)
			{
			  radix1[(statusFileRanges[lPos1][lSize1] >> s1) &
				 0xff]++;
			  radix2[(statusFileAddLoc[lPos2][lSize2] >> s2) &
				 0xff]++;
			  lSize1 += 2;
			  if (lSize1 == 1 << (lPos1 + 1))
			    {
			      lSize1 = 0;
			      lPos1++;
			    }
			  lSize2++;
			  if (lSize2 == 1 << lPos2)
			    {
			      lSize2 = 0;
			      lPos2++;
			    }
			}
		      if (selectedLength1 > selectedLength2)
			{
			  for (m = selectedLength1 - selectedLength2; m; m--)
			    {
			      radix1[(statusFileRanges[lPos1][lSize1] >> s1) &
				     0xff]++;
			      lSize1 += 2;
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			}
		      else
			{
			  for (m = selectedLength2 - selectedLength1; m; m--)
			    {
			      radix2[(statusFileAddLoc[lPos2][lSize2] >> s2) &
				     0xff]++;
			      lSize2++;
			      if (lSize2 == 1 << lPos2)
				{
				  lSize2 = 0;
				  lPos2++;
				}
			    }
			}
		      lPos1 = statusFileRangesPos;
		      lSize1 = statusFileRangesSize;
		      lPos2 = statusFileAddLocPos;
		      lSize2 = statusFileAddLocSize;
		      for (m = 0; m != 256; m++)
			{
			  radixoffPos1[m] = lPos1;
			  radixoffSize1[m] = lSize1;
			  radixoffPos2[m] = lPos2;
			  radixoffSize2[m] = lSize2;
			  n = radix1[m] * 2;
			  lSize1 += n;
			  if (lSize1 <= 1 << (lPos1 + 1))
			    {
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			  else
			    {
			      lSize1 = (1 << (lPos1 + 1)) - lSize1 + n;
			      n -= lSize1;
			      while (n != 0)
				{
				  lPos1++;
				  if (n > 1 << (lPos1 + 1))
				    {
				    }
				  else
				    {
				      lSize1 = n - (1 << (lPos1 + 1));
				      n = 1 << (lPos1 + 1);
				    }
				  lSize1 += 1 << (lPos1 + 1);
				  n -= 1 << (lPos1 + 1);
				}
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			  n = radix2[m];
			  lSize2 += n;
			  if (lSize2 <= 1 << lPos2)
			    {
			      if (lSize2 == 1 << lPos2)
				{
				  lSize2 = 0;
				  lPos2++;
				}
			    }
			  else
			    {
			      lSize2 = (1 << lPos2) - lSize2 + n;
			      n -= lSize2;
			      while (n != 0)
				{
				  lPos2++;
				  if (n > 1 << lPos2)
				    {
				    }
				  else
				    {
				      lSize2 = n - (1 << lPos2);
				      n = 1 << lPos2;
				    }
				  lSize2 += 1 << lPos2;
				  n -= 1 << lPos2;
				}
			      if (lSize2 == 1 << lPos2)
				{
				  lPos2++;
				  lSize2 = 0;
				}
			    }
			}
		      lPos1 = statusFileRangesPos;
		      lSize1 = statusFileRangesSize;
		      lPos2 = statusFileAddLocPos;
		      lSize2 = statusFileAddLocSize;
		      for (m = 0; m != 256; m++)
			{
			  n = radix1[m] * 2;
			  lSize1 += n;
			  if (lSize1 <= 1 << (lPos1 + 1))
			    {
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			  else
			    {
			      lSize1 = (1 << (lPos1 + 1)) - lSize1 + n;
			      n -= lSize1;
			      while (n != 0)
				{
				  lPos1++;
				  if (n > 1 << (lPos1 + 1))
				    {
				    }
				  else
				    {
				      lSize1 = n - (1 << (lPos1 + 1));
				      n = 1 << (lPos1 + 1);
				    }
				  lSize1 += 1 << (lPos1 + 1);
				  n -= 1 << (lPos1 + 1);
				}
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			  n = radix2[m];
			  lSize2 += n;
			  if (lSize2 <= 1 << lPos2)
			    {
			      if (lSize2 == 1 << lPos2)
				{
				  lSize2 = 0;
				  lPos2++;
				}
			    }
			  else
			    {
			      lSize2 = (1 << lPos2) - lSize2 + n;
			      n -= lSize2;
			      while (n != 0)
				{
				  lPos2++;
				  if (n > 1 << lPos2)
				    {
				    }
				  else
				    {
				      lSize2 = n - (1 << lPos2);
				      n = 1 << lPos2;
				    }
				  lSize2 += 1 << lPos2;
				  n -= 1 << lPos2;
				}
			      if (lSize2 == 1 << lPos2)
				{
				  lPos2++;
				  lSize2 = 0;
				}
			    }
			  while ((radixoffPos1[m] != lPos1
				  || radixoffSize1[m] != lSize1)
				 && (radixoffPos2[m] != lPos2
				     || radixoffSize2[m] != lSize2))
			    {
			      n =
				statusFileRanges[radixoffPos1[m]]
				[radixoffSize1[m]];
			      o =
				statusFileRanges[radixoffPos1[m]]
				[radixoffSize1[m] + 1];
			      p =
				statusFileAddLoc[radixoffPos2[m]]
				[radixoffSize2[m]];
			      q = offset[radixoffPos2[m]][radixoffSize2[m]];
			      statusFileRanges[radixoffPos1[m]][radixoffSize1
								[m]] =
				statusFileRanges[radixoffPos1
						 [(n >> s1) &
						  0xff]][radixoffSize1[(n >>
									s1) &
								       0xff]];
			      statusFileRanges[radixoffPos1[m]][radixoffSize1
								[m] + 1] =
				statusFileRanges[radixoffPos1
						 [(n >> s1) &
						  0xff]][radixoffSize1[(n >>
									s1) &
								       0xff] +
							 1];
			      statusFileAddLoc[radixoffPos2[m]][radixoffSize2
								[m]] =
				statusFileAddLoc[radixoffPos2
						 [(p >> s2) &
						  0xff]][radixoffSize2[(p >>
									s2) &
								       0xff]];
			      offset[radixoffPos2[m]][radixoffSize2[m]] =
				offset[radixoffPos2[(p >> s2) & 0xff]]
				[radixoffSize2[(p >> s2) & 0xff]];
			      statusFileRanges[radixoffPos1[(n >> s1) & 0xff]]
				[radixoffSize1[(n >> s1) & 0xff]] = n;
			      statusFileRanges[radixoffPos1[(n >> s1) & 0xff]]
				[radixoffSize1[(n >> s1) & 0xff] + 1] = o;
			      statusFileAddLoc[radixoffPos2[(p >> s2) & 0xff]]
				[radixoffSize2[(p >> s2) & 0xff]] = p;
			      offset[radixoffPos2[(p >> s2) & 0xff]]
				[radixoffSize2[(p >> s2) & 0xff]] = q;
			      n = (n >> s1) & 0xff;
			      p = (p >> s2) & 0xff;
			      radixoffSize1[n] += 2;
			      radixoffSize2[p]++;
			      if (radixoffSize1[n] ==
				  1 << (radixoffPos1[n] + 1))
				{
				  radixoffSize1[n] = 0;
				  radixoffPos1[n]++;
				}
			      if (radixoffSize2[p] == 1 << radixoffPos2[p])
				{
				  radixoffSize2[p] = 0;
				  radixoffPos2[p]++;
				}
			    }
			  while (radixoffPos1[m] != lPos1
				 || radixoffSize1[m] != lSize1)
			    {
			      n =
				statusFileRanges[radixoffPos1[m]]
				[radixoffSize1[m]];
			      o =
				statusFileRanges[radixoffPos1[m]]
				[radixoffSize1[m] + 1];
			      statusFileRanges[radixoffPos1[m]][radixoffSize1
								[m]] =
				statusFileRanges[radixoffPos1
						 [(n >> s1) &
						  0xff]][radixoffSize1[(n >>
									s1) &
								       0xff]];
			      statusFileRanges[radixoffPos1[m]][radixoffSize1
								[m] + 1] =
				statusFileRanges[radixoffPos1
						 [(n >> s1) &
						  0xff]][radixoffSize1[(n >>
									s1) &
								       0xff] +
							 1];
			      statusFileRanges[radixoffPos1[(n >> s1) & 0xff]]
				[radixoffSize1[(n >> s1) & 0xff]] = n;
			      statusFileRanges[radixoffPos1[(n >> s1) & 0xff]]
				[radixoffSize1[(n >> s1) & 0xff] + 1] = o;
			      n = (n >> s1) & 0xff;
			      radixoffSize1[n] += 2;
			      if (radixoffSize1[n] ==
				  1 << (radixoffPos1[n] + 1))
				{
				  radixoffSize1[n] = 0;
				  radixoffPos1[n]++;
				}
			    }
			  while (radixoffPos2[m] != lPos2
				 || radixoffSize2[m] != lSize2)
			    {
			      p =
				statusFileAddLoc[radixoffPos2[m]]
				[radixoffSize2[m]];
			      q = offset[radixoffPos2[m]][radixoffSize2[m]];
			      statusFileAddLoc[radixoffPos2[m]][radixoffSize2
								[m]] =
				statusFileAddLoc[radixoffPos2
						 [(p >> s2) &
						  0xff]][radixoffSize2[(p >>
									s2) &
								       0xff]];
			      offset[radixoffPos2[m]][radixoffSize2[m]] =
				offset[radixoffPos2[(p >> s2) & 0xff]]
				[radixoffSize2[(p >> s2) & 0xff]];
			      statusFileAddLoc[radixoffPos2[(p >> s2) & 0xff]]
				[radixoffSize2[(p >> s2) & 0xff]] = p;
			      offset[radixoffPos2[(p >> s2) & 0xff]]
				[radixoffSize2[(p >> s2) & 0xff]] = q;
			      p = (p >> s2) & 0xff;
			      radixoffSize2[p]++;
			      if (radixoffSize2[p] == 1 << radixoffPos2[p])
				{
				  radixoffSize2[p] = 0;
				  radixoffPos2[p]++;
				}
			    }
			}
		      selectedLength1 = radix1[(i >> s1) & 0xff];
		      selectedLength2 = radix2[(j >> s2) & 0xff];
		      if (k == sizeof (i) - 1
			  && (lPos1 != statusFileRangesPosMax
			      || lSize1 != statusFileRangesSizeMax))
			{
			  i |= 0xff;
			  statusFileRangesPos = lPos1;
			  statusFileRangesSize = lSize1;
			  while (s1 != sizeof (i) * 8
				 && ((i >> s1) & 0xff) == 0xff)
			    {
			      s1 += 8;
			      i >>= s1;
			      i++;
			      i <<= s1;
			      radix1 -= 256;
			      k--;
			    }
			  selectedLength1 = radix1[(i >> s1) & 0xff];
			}
		      if (l == sizeof (j) - 1
			  && (lPos2 != statusFileAddLocPosMax
			      || lSize2 != statusFileAddLocSizeMax))
			{
			  j |= 0xff;
			  statusFileAddLocPos = lPos2;
			  statusFileAddLocSize = lSize2;
			  while (s2 != sizeof (j) * 8
				 && ((j >> s2) & 0xff) == 0xff)
			    {
			      s2 += 8;
			      j >>= s2;
			      j++;
			      j <<= s2;
			      radix2 -= 256;
			      l--;
			    }
			  selectedLength2 = radix2[(j >> s2) & 0xff];
			}
		      if (!selectedLength1)
			{
			  while (!selectedLength1 && s1 != sizeof (i) * 8)
			    {
			      i >>= s1;
			      for (m = i & 0xff; !selectedLength1 && m != 256;
				   m++)
				{
				  selectedLength1 = radix1[m];
				}
			      i >>= 8;
			      i <<= 8;
			      i += m - 1;
			      i <<= s1;
			      if (m == 256)
				{
				  s1 += 8;
				  i >>= s1;
				  i++;
				  i <<= s1;
				  radix1 -= 256;
				  k--;
				  while (s1 != sizeof (i) * 8
					 && ((i >> s1) & 0xff) == 0xff)
				    {
				      s1 += 8;
				      i >>= s1;
				      i++;
				      i <<= s1;
				      radix1 -= 256;
				      k--;
				    }
				  if (s1 == sizeof (i) * 8)
				    {
				      k = sizeof (i);
				    }
				  else
				    {
				      selectedLength1 =
					radix1[(i >> s1) & 0xff];
				    }
				}
			    }
			}
		      if (!selectedLength2)
			{
			  while (!selectedLength2 && s2 != sizeof (j) * 8)
			    {
			      j >>= s2;
			      for (m = j & 0xff; !selectedLength2 && m != 256;
				   m++)
				{
				  selectedLength2 = radix2[m];
				}
			      j >>= 8;
			      j <<= 8;
			      j += m - 1;
			      j <<= s2;
			      if (m == 256)
				{
				  s2 += 8;
				  j >>= s2;
				  j++;
				  j <<= s2;
				  radix2 -= 256;
				  l--;
				  while (s2 != sizeof (j) * 8
					 && ((j >> s2) & 0xff) == 0xff)
				    {
				      s2 += 8;
				      j >>= s2;
				      j++;
				      j <<= s2;
				      radix2 -= 256;
				      l--;
				    }
				  if (s2 == sizeof (j) * 8)
				    {
				      l = sizeof (j);
				    }
				  else
				    {
				      selectedLength2 =
					radix2[(j >> s2) & 0xff];
				    }
				}
			    }
			}
		      radix1 += 256;
		      radix2 += 256;
		      k++;
		      l++;
		    }
		  while (k != sizeof (i))
		    {
		      for (m = 0; m != 256; m++)
			{
			  radix1[m] = 0;
			}
		      s1 = 64 - 8 - (k * 8);
		      lPos1 = statusFileRangesPos;
		      lSize1 = statusFileRangesSize;
		      for (m = selectedLength1; m; m--)
			{
			  radix1[(statusFileRanges[lPos1][lSize1] >> s1) &
				 0xff]++;
			  lSize1 += 2;
			  if (lSize1 == 1 << (lPos1 + 1))
			    {
			      lSize1 = 0;
			      lPos1++;
			    }
			}
		      lPos1 = statusFileRangesPos;
		      lSize1 = statusFileRangesSize;
		      for (m = 0; m != 256; m++)
			{
			  radixoffPos1[m] = lPos1;
			  radixoffSize1[m] = lSize1;
			  n = radix1[m] * 2;
			  lSize1 += n;
			  if (lSize1 <= 1 << (lPos1 + 1))
			    {
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			  else
			    {
			      lSize1 = (1 << (lPos1 + 1)) - lSize1 + n;
			      n -= lSize1;
			      while (n != 0)
				{
				  lPos1++;
				  if (n > 1 << (lPos1 + 1))
				    {
				    }
				  else
				    {
				      lSize1 = n - (1 << (lPos1 + 1));
				      n = 1 << (lPos1 + 1);
				    }
				  lSize1 += 1 << (lPos1 + 1);
				  n -= 1 << (lPos1 + 1);
				}
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			}
		      lPos1 = statusFileRangesPos;
		      lSize1 = statusFileRangesSize;
		      for (m = 0; m != 256; m++)
			{
			  n = radix1[m] * 2;
			  lSize1 += n;
			  if (lSize1 <= 1 << (lPos1 + 1))
			    {
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			  else
			    {
			      lSize1 = (1 << (lPos1 + 1)) - lSize1 + n;
			      n -= lSize1;
			      while (n != 0)
				{
				  lPos1++;
				  if (n > 1 << (lPos1 + 1))
				    {
				    }
				  else
				    {
				      lSize1 = n - (1 << (lPos1 + 1));
				      n = 1 << (lPos1 + 1);
				    }
				  lSize1 += 1 << (lPos1 + 1);
				  n -= 1 << (lPos1 + 1);
				}
			      if (lSize1 == 1 << (lPos1 + 1))
				{
				  lSize1 = 0;
				  lPos1++;
				}
			    }
			  while (radixoffPos1[m] != lPos1
				 || radixoffSize1[m] != lSize1)
			    {
			      n =
				statusFileRanges[radixoffPos1[m]]
				[radixoffSize1[m]];
			      o =
				statusFileRanges[radixoffPos1[m]]
				[radixoffSize1[m] + 1];
			      statusFileRanges[radixoffPos1[m]][radixoffSize1
								[m]] =
				statusFileRanges[radixoffPos1
						 [(n >> s1) &
						  0xff]][radixoffSize1[(n >>
									s1) &
								       0xff]];
			      statusFileRanges[radixoffPos1[m]][radixoffSize1
								[m] + 1] =
				statusFileRanges[radixoffPos1
						 [(n >> s1) &
						  0xff]][radixoffSize1[(n >>
									s1) &
								       0xff] +
							 1];
			      statusFileRanges[radixoffPos1[(n >> s1) & 0xff]]
				[radixoffSize1[(n >> s1) & 0xff]] = n;
			      statusFileRanges[radixoffPos1[(n >> s1) & 0xff]]
				[radixoffSize1[(n >> s1) & 0xff] + 1] = o;
			      n = (n >> s1) & 0xff;
			      radixoffSize1[n] += 2;
			      if (radixoffSize1[n] ==
				  1 << (radixoffPos1[n] + 1))
				{
				  radixoffSize1[n] = 0;
				  radixoffPos1[n]++;
				}
			    }
			}
		      selectedLength1 = radix1[(i >> s1) & 0xff];
		      if (k == sizeof (i) - 1
			  && (lPos1 != statusFileRangesPosMax
			      || lSize1 != statusFileRangesSizeMax))
			{
			  i |= 0xff;
			  statusFileRangesPos = lPos1;
			  statusFileRangesSize = lSize1;
			  while (s1 != sizeof (i) * 8
				 && ((i >> s1) & 0xff) == 0xff)
			    {
			      s1 += 8;
			      i >>= s1;
			      i++;
			      i <<= s1;
			      radix1 -= 256;
			      k--;
			    }
			  selectedLength1 = radix1[(i >> s1) & 0xff];
			}
		      if (!selectedLength1)
			{
			  while (!selectedLength1 && s1 != sizeof (i) * 8)
			    {
			      i >>= s1;
			      for (m = i & 0xff; !selectedLength1 && m != 256;
				   m++)
				{
				  selectedLength1 = radix1[m];
				}
			      i >>= 8;
			      i <<= 8;
			      i += m - 1;
			      i <<= s1;
			      if (m == 256)
				{
				  s1 += 8;
				  i >>= s1;
				  i++;
				  i <<= s1;
				  radix1 -= 256;
				  k--;
				  while (s1 != sizeof (i) * 8
					 && ((i >> s1) & 0xff) == 0xff)
				    {
				      s1 += 8;
				      i >>= s1;
				      i++;
				      i <<= s1;
				      radix1 -= 256;
				      k--;
				    }
				  if (s1 == sizeof (i) * 8)
				    {
				      k = sizeof (i);
				    }
				  else
				    {
				      selectedLength1 =
					radix1[(i >> s1) & 0xff];
				    }
				}
			    }
			}
		      radix1 += 256;
		      k++;
		    }
		  while (l != sizeof (j))
		    {
		      for (m = 0; m != 256; m++)
			{
			  radix2[m] = 0;
			}
		      s2 = 64 - 8 - (l * 8);
		      lPos2 = statusFileAddLocPos;
		      lSize2 = statusFileAddLocSize;
		      for (m = selectedLength2; m; m--)
			{
			  radix2[(statusFileAddLoc[lPos2][lSize2] >> s2) &
				 0xff]++;
			  lSize2++;
			  if (lSize2 == 1 << lPos2)
			    {
			      lSize2 = 0;
			      lPos2++;
			    }
			}
		      lPos2 = statusFileAddLocPos;
		      lSize2 = statusFileAddLocSize;
		      for (m = 0; m != 256; m++)
			{
			  radixoffPos2[m] = lPos2;
			  radixoffSize2[m] = lSize2;
			  n = radix2[m];
			  lSize2 += n;
			  if (lSize2 <= 1 << lPos2)
			    {
			      if (lSize2 == 1 << lPos2)
				{
				  lSize2 = 0;
				  lPos2++;
				}
			    }
			  else
			    {
			      lSize2 = (1 << lPos2) - lSize2 + n;
			      n -= lSize2;
			      while (n != 0)
				{
				  lPos2++;
				  if (n > 1 << lPos2)
				    {
				    }
				  else
				    {
				      lSize2 = n - (1 << lPos2);
				      n = 1 << lPos2;
				    }
				  lSize2 += 1 << lPos2;
				  n -= 1 << lPos2;
				}
			      if (lSize2 == 1 << lPos2)
				{
				  lPos2++;
				  lSize2 = 0;
				}
			    }
			}
		      lPos2 = statusFileAddLocPos;
		      lSize2 = statusFileAddLocSize;
		      for (m = 0; m != 256; m++)
			{
			  n = radix2[m];
			  lSize2 += n;
			  if (lSize2 <= 1 << lPos2)
			    {
			      if (lSize2 == 1 << lPos2)
				{
				  lSize2 = 0;
				  lPos2++;
				}
			    }
			  else
			    {
			      lSize2 = (1 << lPos2) - lSize2 + n;
			      n -= lSize2;
			      while (n != 0)
				{
				  lPos2++;
				  if (n > 1 << lPos2)
				    {
				    }
				  else
				    {
				      lSize2 = n - (1 << lPos2);
				      n = 1 << lPos2;
				    }
				  lSize2 += 1 << lPos2;
				  n -= 1 << lPos2;
				}
			      if (lSize2 == 1 << lPos2)
				{
				  lPos2++;
				  lSize2 = 0;
				}
			    }
			  while (radixoffPos2[m] != lPos2
				 || radixoffSize2[m] != lSize2)
			    {
			      p =
				statusFileAddLoc[radixoffPos2[m]]
				[radixoffSize2[m]];
			      q = offset[radixoffPos2[m]][radixoffSize2[m]];
			      statusFileAddLoc[radixoffPos2[m]][radixoffSize2
								[m]] =
				statusFileAddLoc[radixoffPos2
						 [(p >> s2) &
						  0xff]][radixoffSize2[(p >>
									s2) &
								       0xff]];
			      offset[radixoffPos2[m]][radixoffSize2[m]] =
				offset[radixoffPos2[(p >> s2) & 0xff]]
				[radixoffSize2[(p >> s2) & 0xff]];
			      statusFileAddLoc[radixoffPos2[(p >> s2) & 0xff]]
				[radixoffSize2[(p >> s2) & 0xff]] = p;
			      offset[radixoffPos2[(p >> s2) & 0xff]]
				[radixoffSize2[(p >> s2) & 0xff]] = q;
			      p = (p >> s2) & 0xff;
			      radixoffSize2[p]++;
			      if (radixoffSize2[p] == 1 << radixoffPos2[p])
				{
				  radixoffSize2[p] = 0;
				  radixoffPos2[p]++;
				}
			    }
			}
		      selectedLength2 = radix2[(j >> s2) & 0xff];
		      if (l == sizeof (j) - 1
			  && (lPos2 != statusFileAddLocPosMax
			      || lSize2 != statusFileAddLocSizeMax))
			{
			  j |= 0xff;
			  statusFileAddLocPos = lPos2;
			  statusFileAddLocSize = lSize2;
			  while (s2 != sizeof (j) * 8
				 && ((j >> s2) & 0xff) == 0xff)
			    {
			      s2 += 8;
			      j >>= s2;
			      j++;
			      j <<= s2;
			      radix2 -= 256;
			      l--;
			    }
			  selectedLength2 = radix2[(j >> s2) & 0xff];
			}
		      if (!selectedLength2)
			{
			  while (!selectedLength2 && s2 != sizeof (j) * 8)
			    {
			      j >>= s2;
			      for (m = j & 0xff; !selectedLength2 && m != 256;
				   m++)
				{
				  selectedLength2 = radix2[m];
				}
			      j >>= 8;
			      j <<= 8;
			      j += m - 1;
			      j <<= s2;
			      if (m == 256)
				{
				  s2 += 8;
				  j >>= s2;
				  j++;
				  j <<= s2;
				  radix2 -= 256;
				  l--;
				  while (s2 != sizeof (j) * 8
					 && ((j >> s2) & 0xff) == 0xff)
				    {
				      s2 += 8;
				      j >>= s2;
				      j++;
				      j <<= s2;
				      radix2 -= 256;
				      l--;
				    }
				  if (s2 == sizeof (j) * 8)
				    {
				      l = sizeof (j);
				    }
				  else
				    {
				      selectedLength2 =
					radix2[(j >> s2) & 0xff];
				    }
				}
			    }
			}
		      radix2 += 256;
		      l++;
		    }
		  statusFileAddStrPos = statusFileAddStrSize = 0;
		  for (i = 0; i != statusFileAddLocPosMax; i++)
		    {
		      for (j = 0; j != 1 << i; j++)
			{
			  k = statusFileAddSize[i][j];
			  statusFileAddSize[i][j] =
			    (statusFileAddStrPos <<
			     (sizeof (statusFileAddSize[i][j]) * 8 - 6)) +
			    statusFileAddStrSize;
			  statusFileAddStrSize += k;
			  if (statusFileAddStrSize <=
			      1 << statusFileAddStrPos)
			    {
			      if (statusFileAddStrSize ==
				  1 << statusFileAddStrPos)
				{
				  statusFileAddStrSize = 0;
				  statusFileAddStrPos++;
				}
			    }
			  else
			    {
			      radixoffPos1[statusFileAddStrPos] = i;
			      radixoffSize1[statusFileAddStrPos] = j;
			      radixoffPos2[statusFileAddStrPos] =
				statusFileAddSize[i][j];
			      radixoffSize2[statusFileAddStrPos] = k;
			      statusFileAddStrSize =
				(1 << statusFileAddStrPos) -
				statusFileAddStrSize + k;
			      k -= statusFileAddStrSize;
			      while (k != 0)
				{
				  statusFileAddStrPos++;
				  if (k > 1 << statusFileAddStrPos)
				    {
				    }
				  else
				    {
				      statusFileAddStrSize =
					k - (1 << statusFileAddStrPos);
				      k = 1 << statusFileAddStrPos;
				    }
				  statusFileAddStrSize +=
				    1 << statusFileAddStrPos;
				  k -= 1 << statusFileAddStrPos;
				}
			      if (statusFileAddStrSize ==
				  1 << statusFileAddStrPos)
				{
				  statusFileAddStrPos++;
				  statusFileAddStrSize = 0;
				}
			    }
			}
		    }
		  if (statusFileAddLocSizeMax)
		    {
		      for (j = 0; j != statusFileAddLocSizeMax; j++)
			{
			  k = statusFileAddSize[i][j];
			  statusFileAddSize[i][j] =
			    (statusFileAddStrPos <<
			     (sizeof (statusFileAddSize[i][j]) * 8 - 6)) +
			    statusFileAddStrSize;
			  statusFileAddStrSize += k;
			  if (statusFileAddStrSize <=
			      1 << statusFileAddStrPos)
			    {
			      if (statusFileAddStrSize ==
				  1 << statusFileAddStrPos)
				{
				  statusFileAddStrSize = 0;
				  statusFileAddStrPos++;
				}
			    }
			  else
			    {
			      radixoffPos1[statusFileAddStrPos] = i;
			      radixoffSize1[statusFileAddStrPos] = j;
			      radixoffPos2[statusFileAddStrPos] =
				statusFileAddSize[i][j];
			      radixoffSize2[statusFileAddStrPos] = k;
			      statusFileAddStrSize =
				(1 << statusFileAddStrPos) -
				statusFileAddStrSize + k;
			      k -= statusFileAddStrSize;
			      while (k != 0)
				{
				  statusFileAddStrPos++;
				  if (k > 1 << statusFileAddStrPos)
				    {
				    }
				  else
				    {
				      statusFileAddStrSize =
					k - (1 << statusFileAddStrPos);
				      k = 1 << statusFileAddStrPos;
				    }
				  statusFileAddStrSize +=
				    1 << statusFileAddStrPos;
				  k -= 1 << statusFileAddStrPos;
				}
			      if (statusFileAddStrSize ==
				  1 << statusFileAddStrPos)
				{
				  statusFileAddStrPos++;
				  statusFileAddStrSize = 0;
				}
			    }
			}
		    }
		  statusFileAddSize[statusFileAddLocPosMax]
		    [statusFileAddLocSizeMax] =
		    (statusFileAddStrPos <<
		     (sizeof (statusFileAddSize[i][j]) * 8 - 6)) +
		    statusFileAddStrSize;
		  for (k = i = 0; i != statusFileAddLocPosMax; i++)
		    {
		      for (j = 0; j != 1 << i; j++)
			{
			  l = offset[i][j] >> (sizeof (offset[i][j]) * 8 - 6);
			  m = 0;
			  m--;
			  m >>= 6;
			  m = offset[i][j] & m;
			  statusFileAddStRSize[k] = statusFileAddSize[l][m];
			  n = statusFileAddSize[l][m];
			  m++;
			  if (m == 1 << l)
			    {
			      m = 0;
			      l++;
			    }
			  l = statusFileAddSize[l][m];
			  m = 0;
			  m--;
			  m >>= 6;
			  m = l & m;
			  l >>= (sizeof (statusFileAddSize[l][m]) * 8 - 6);
			  o = 0;
			  o--;
			  o >>= 6;
			  o = n & o;
			  offset[i][j] = n;
			  n >>= (sizeof (statusFileAddSize[l][m]) * 8 - 6);
			  if (n == l)
			    {
			      statusFileAddStRSize[k] = m - o;
			    }
			  else
			    {
			      for (p = 0; n != l; n++)
				{
				  p += 1 << n;
				}
			      statusFileAddStRSize[k] = p + m - o;
			    }
			  k++;
			}
		    }
		  for (j = 0; j != statusFileAddLocSizeMax; j++)
		    {
		      l = offset[i][j] >> (sizeof (offset[i][j]) * 8 - 6);
		      m = 0;
		      m--;
		      m >>= 6;
		      m = offset[i][j] & m;
		      statusFileAddStRSize[k] = statusFileAddSize[l][m];
		      n = statusFileAddSize[l][m];
		      m++;
		      if (m == 1 << l)
			{
			  m = 0;
			  l++;
			}
		      l = statusFileAddSize[l][m];
		      m = 0;
		      m--;
		      m >>= 6;
		      m = l & m;
		      l >>= (sizeof (statusFileAddSize[l][m]) * 8 - 6);
		      o = 0;
		      o--;
		      o >>= 6;
		      o = n & o;
		      offset[i][j] = n;
		      n >>= (sizeof (statusFileAddSize[l][m]) * 8 - 6);
		      if (n == l)
			{
			  statusFileAddStRSize[k] = m - o;
			}
		      else
			{
			  for (p = 0; n != l; n++)
			    {
			      p += 1 << n;
			    }
			  statusFileAddStRSize[k] = p + m - o;
			}
		      k++;
		    }
		  statusFileAddLocPos = statusFileAddLocSize =
		    statusFileRangesPos = statusFileRangesSize = 0;
		  while (statusFileAddLocPos != statusFileAddLocPosMax
			 || statusFileAddLocSize != statusFileAddLocSizeMax)
		    {
		      statusFileAddLocSize++;
		      if (statusFileAddLocSize == 1 << statusFileAddLocPos)
			{
			  statusFileAddLocSize = 0;
			  statusFileAddLocPos++;
			}
		    }
		}
	      statusFileAddLocPos = statusFileAddLocSize =
		statusFileRangesPos = statusFileRangesSize = 0;
	      statusFileAddStrPos = statusFileAddStrSize = 0;
	      j = 0;
	      k = 0;
	      deloccured1 = statusFileAddLocPosMax || statusFileAddLocSizeMax;
	      if (deloccured1)
		{
		  if (!statusFileAddLocSizeMax)
		    {
		      statusFileAddLocPosMax--;
		      statusFileAddLocSizeMax = 1 << statusFileAddLocPosMax;
		    }
		  statusFileAddLocSizeMax--;
		  if (!statusFileRangesSizeMax)
		    {
		      statusFileRangesPosMax--;
		      statusFileRangesSizeMax =
			1 << (statusFileRangesPosMax + 1);
		    }
		  statusFileRangesSizeMax -= 2;
		  deloccured1 &=
		    statusFileAddLoc[statusFileAddLocPosMax]
		    [statusFileAddLocSizeMax] ==
		    statusFileRanges[statusFileRangesPosMax]
		    [statusFileRangesSizeMax];
		  if (!deloccured1)
		    {
		      statusFileAddLocSizeMax++;
		    }
		  if (!deloccured1
		      && statusFileAddLocSizeMax ==
		      1 << statusFileAddLocPosMax)
		    {
		      statusFileAddLocSizeMax = 0;
		      statusFileAddLocPosMax++;
		    }
		  statusFileRangesSizeMax += 2;
		  if (statusFileRangesSizeMax ==
		      1 << (statusFileRangesPosMax + 1))
		    {
		      statusFileRangesSizeMax = 0;
		      statusFileRangesPosMax++;
		    }
		}
	      if (statusFileRequestSorted)
		{
		  for (i = 0; i != statusFileSize - 1; i++)
		    {
		      if (!
			  (statusFileAddLocPos == statusFileAddLocPosMax
			   && statusFileAddLocSize == statusFileAddLocSizeMax)
			  &&
			  statusFileAddLoc[statusFileAddLocPos]
			  [statusFileAddLocSize] <
			  statusFileRanges[statusFileRangesPos]
			  [statusFileRangesSize])
			{
			  if (j <
			      statusFileAddSize[statusFileAddLocPos]
			      [statusFileAddLocSize])
			    {
			      i = j;
			      j =
				statusFileAddSize[statusFileAddLocPos]
				[statusFileAddLocSize] - i;
			      n =
				statusFileAddSize[statusFileAddLocPos]
				[statusFileAddLocSize];
			      remaining =
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize + 1] -
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize];
			      while (j > remaining)
				{
				  j -= remaining;
				  statusFileRangesSize += 2;
				  if (statusFileRangesSize ==
				      1 << (statusFileRangesPos + 1))
				    {
				      statusFileRangesSize = 0;
				      statusFileRangesPos++;
				    }
				  remaining =
				    statusFileRanges[statusFileRangesPos]
				    [statusFileRangesSize + 1] -
				    statusFileRanges[statusFileRangesPos]
				    [statusFileRangesSize];
				}
			      statusFileAddStrSize += n;
			      if (statusFileAddStrSize <=
				  1 << statusFileAddStrPos)
				{
				  if (statusFileAddStrSize ==
				      1 << statusFileAddStrPos)
				    {
				      statusFileAddStrSize = 0;
				      statusFileAddStrPos++;
				    }
				}
			      else
				{
				  statusFileAddStrSize =
				    (1 << statusFileAddStrPos) -
				    statusFileAddStrSize + n;
				  n -= statusFileAddStrSize;
				  while (n != 0)
				    {
				      statusFileAddStrPos++;
				      if (n > 1 << statusFileAddStrPos)
					{
					}
				      else
					{
					  statusFileAddStrSize =
					    n - (1 << statusFileAddStrPos);
					  n = 1 << statusFileAddStrPos;
					}
				      statusFileAddStrSize +=
					1 << statusFileAddStrPos;
				      n -= 1 << statusFileAddStrPos;
				    }
				  if (statusFileAddStrSize ==
				      1 << statusFileAddStrPos)
				    {
				      statusFileAddStrPos++;
				      statusFileAddStrSize = 0;
				    }
				}
			      statusFileAddLocSize++;
			      if (statusFileAddLocSize ==
				  1 << statusFileAddLocPos)
				{
				  statusFileAddLocSize = 0;
				  statusFileAddLocPos++;
				}
			      l = 0;
			      for (k = 1;
				   !(statusFileAddLocPos ==
				     statusFileAddLocPosMax
				     && statusFileAddLocSize ==
				     statusFileAddLocSizeMax)
				   && (j > remaining
				       ||
				       statusFileAddLoc[statusFileAddLocPos]
				       [statusFileAddLocSize] <
				       statusFileRanges[statusFileRangesPos]
				       [statusFileRangesSize + 1]); k++)
				{
				  j +=
				    statusFileAddSize[statusFileAddLocPos]
				    [statusFileAddLocSize];
				  n =
				    statusFileAddSize[statusFileAddLocPos]
				    [statusFileAddLocSize];
				  while (j > remaining)
				    {
				      j -= remaining;
				      statusFileRangesSize += 2;
				      if (statusFileRangesSize ==
					  1 << (statusFileRangesPos + 1))
					{
					  statusFileRangesSize = 0;
					  statusFileRangesPos++;
					}
				      remaining =
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize + 1] -
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize];
				    }
				  statusFileAddStrSize += n;
				  if (statusFileAddStrSize <=
				      1 << statusFileAddStrPos)
				    {
				      if (statusFileAddStrSize ==
					  1 << statusFileAddStrPos)
					{
					  statusFileAddStrSize = 0;
					  statusFileAddStrPos++;
					}
				    }
				  else
				    {
				      statusFileAddStrSize =
					(1 << statusFileAddStrPos) -
					statusFileAddStrSize + n;
				      n -= statusFileAddStrSize;
				      while (n != 0)
					{
					  statusFileAddStrPos++;
					  if (n > 1 << statusFileAddStrPos)
					    {
					    }
					  else
					    {
					      statusFileAddStrSize =
						n -
						(1 << statusFileAddStrPos);
					      n = 1 << statusFileAddStrPos;
					    }
					  statusFileAddStrSize +=
					    1 << statusFileAddStrPos;
					  n -= 1 << statusFileAddStrPos;
					}
				      if (statusFileAddStrSize ==
					  1 << statusFileAddStrPos)
					{
					  statusFileAddStrPos++;
					  statusFileAddStrSize = 0;
					}
				    }
				  statusFileAddLocSize++;
				  if (statusFileAddLocSize ==
				      1 << statusFileAddLocPos)
				    {
				      statusFileAddLocSize = 0;
				      statusFileAddLocPos++;
				    }
				}
			      j = remaining - j;
			      o = statusFileRangesPos;
			      p = statusFileRangesSize;
			      q = statusFileAddLocPos;
			      r = statusFileAddLocSize;
			      s = statusFileAddStrPos;
			      t = statusFileAddStrSize;
			      if (statusFileAddLocSize == 0)
				{
				  statusFileAddLocPos--;
				  statusFileAddLocSize =
				    1 << statusFileAddLocPos;
				}
			      statusFileAddLocSize--;
			      m =
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize + 1] - 1 - j;
			      l =
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize] - 1;
			      remaining = 0;
			      if (!
				  (statusFileRangesPos | statusFileRangesSize
				   | remaining)
				  &&
				  statusFileAddLoc[statusFileAddLocPos]
				  [statusFileAddLocSize] <
				  statusFileRanges[statusFileRangesPos]
				  [statusFileRangesSize + 1])
				{
				  remaining--;
				}
			      if ((statusFileRangesPos | statusFileRangesSize)
				  &&
				  statusFileAddLoc[statusFileAddLocPos]
				  [statusFileAddLocSize] <
				  statusFileRanges[statusFileRangesPos]
				  [statusFileRangesSize + 1])
				{
				  if (statusFileRangesPos)
				    {
				      if (!statusFileRangesSize)
					{
					  statusFileRangesPos--;
					  statusFileRangesSize =
					    1 << (statusFileRangesPos + 1);
					}
				    }
				  else if (!statusFileRangesSize)
				    {
				      statusFileRangesSize = 2;
				    }
				  statusFileRangesSize -= 2;
				}
			      for (; k; k--)
				{
				  while ((statusFileRangesPos |
					  statusFileRangesSize)
					 &&
					 statusFileAddLoc[statusFileAddLocPos]
					 [statusFileAddLocSize] <
					 statusFileRanges[statusFileRangesPos]
					 [statusFileRangesSize + 1])
				    {
				      for (;
					   l !=
					   statusFileRanges
					   [statusFileRangesPos]
					   [statusFileRangesSize + 1] - 1;
					   l--)
					{
					  statusFileMemAlloc[m] =
					    statusFileMemAlloc[l];
					  m--;
					}
				      l =
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize] - 1;
				      if (statusFileRangesPos)
					{
					  if (!statusFileRangesSize)
					    {
					      statusFileRangesPos--;
					      statusFileRangesSize =
						1 << (statusFileRangesPos +
						      1);
					    }
					}
				      else if (!statusFileRangesSize)
					{
					  statusFileRangesSize = 2;
					}
				      statusFileRangesSize -= 2;
				    }
				  if (!
				      (statusFileRangesPos |
				       statusFileRangesSize | remaining)
				      &&
				      statusFileAddLoc[statusFileAddLocPos]
				      [statusFileAddLocSize] <
				      statusFileRanges[statusFileRangesPos]
				      [statusFileRangesSize + 1])
				    {
				      for (;
					   l !=
					   statusFileRanges
					   [statusFileRangesPos]
					   [statusFileRangesSize + 1] - 1;
					   l--)
					{
					  statusFileMemAlloc[m] =
					    statusFileMemAlloc[l];
					  m--;
					}
				      l =
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize] - 1;
				      remaining--;
				    }
				  for (;
				       l !=
				       statusFileAddLoc[statusFileAddLocPos]
				       [statusFileAddLocSize] - 1; l--)
				    {
				      statusFileMemAlloc[m] =
					statusFileMemAlloc[l];
				      m--;
				    }
				  m++;
				  n =
				    statusFileAddSize[statusFileAddLocPos]
				    [statusFileAddLocSize];
				  if (statusFileAddStrSize >= n)
				    {
				      statusFileAddStrSize -= n;
				      m -= n;
				      memcpy (statusFileMemAlloc + m,
					      statusFileAddStr
					      [statusFileAddStrPos] +
					      statusFileAddStrSize, n);
				      if (statusFileAddStrSize == 0)
					{
					  statusFileAddStrPos--;
					  statusFileAddStrSize =
					    1 << statusFileAddStrPos;
					}
				    }
				  else
				    {
				      n -= statusFileAddStrSize;
				      m -= statusFileAddStrSize;
				      memcpy (statusFileMemAlloc + m,
					      statusFileAddStr
					      [statusFileAddStrPos],
					      statusFileAddStrSize);
				      while (n != 0)
					{
					  statusFileAddStrPos--;
					  if (n > (1 << statusFileAddStrPos))
					    {
					      n -= (1 << statusFileAddStrPos);
					      m -= (1 << statusFileAddStrPos);
					      memcpy (statusFileMemAlloc + m,
						      statusFileAddStr
						      [statusFileAddStrPos],
						      (1 <<
						       statusFileAddStrPos));
					    }
					  else
					    {
					      statusFileAddStrSize =
						(1 << statusFileAddStrPos) -
						n;
					      m -= n;
					      memcpy (statusFileMemAlloc + m,
						      statusFileAddStr
						      [statusFileAddStrPos] +
						      statusFileAddStrSize,
						      n);
					      n = 0;
					    }
					}
				      if (statusFileAddStrSize == 0)
					{
					  statusFileAddStrPos--;
					  statusFileAddStrSize =
					    1 << statusFileAddStrPos;
					}
				    }
				  m--;
				  if (statusFileAddLocSize == 0)
				    {
				      statusFileAddLocPos--;
				      statusFileAddLocSize =
					1 << statusFileAddLocPos;
				    }
				  statusFileAddLocSize--;
				}
			      statusFileRangesPos = o;
			      statusFileRangesSize = p;
			      statusFileAddLocPos = q;
			      statusFileAddLocSize = r;
			      statusFileAddStrPos = s;
			      statusFileAddStrSize = t;
			      i =
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize + 1] - j;
			      statusFileRangesSize += 2;
			      if (statusFileRangesSize ==
				  1 << (statusFileRangesPos + 1))
				{
				  statusFileRangesSize = 0;
				  statusFileRangesPos++;
				}
			      if (j
				  && !(statusFileRangesPos ==
				       statusFileRangesPosMax
				       && statusFileRangesSize ==
				       statusFileRangesSizeMax
				       && statusFileAddLocPos ==
				       statusFileAddLocPosMax
				       && statusFileAddLocSize ==
				       statusFileAddLocSizeMax))
				{
				  if (!
				      (statusFileRangesPos ==
				       statusFileRangesPosMax
				       && statusFileRangesSize ==
				       statusFileRangesSizeMax)
				      &&
				      ((statusFileAddLocPos ==
					statusFileAddLocPosMax
					&& statusFileAddLocSize ==
					statusFileAddLocSizeMax)
				       ||
				       statusFileRanges[statusFileRangesPos]
				       [statusFileRangesSize + 1] <
				       statusFileAddLoc[statusFileAddLocPos]
				       [statusFileAddLocSize]))
				    {
				      if (!
					  (statusFileRangesPos ==
					   statusFileRangesPosMax
					   && statusFileRangesSize ==
					   statusFileRangesSizeMax))
					{
					  for (k = i + j;
					       k !=
					       statusFileRanges
					       [statusFileRangesPos]
					       [statusFileRangesSize]; k++)
					    {
					      statusFileMemAlloc[i] =
						statusFileMemAlloc[k];
					      i++;
					    }
					  if (statusFileAddLocPos ==
					      statusFileAddLocPosMax
					      && statusFileAddLocSize ==
					      statusFileAddLocSizeMax)
					    {
					      j +=
						statusFileRanges
						[statusFileRangesPos]
						[statusFileRangesSize + 1] -
						statusFileRanges
						[statusFileRangesPos]
						[statusFileRangesSize];
					      statusFileRangesSize += 2;
					      if (statusFileRangesSize ==
						  1 << (statusFileRangesPos +
							1))
						{
						  statusFileRangesSize = 0;
						  statusFileRangesPos++;
						}
					      while (!
						     (statusFileRangesPos ==
						      statusFileRangesPosMax
						      && statusFileRangesSize
						      ==
						      statusFileRangesSizeMax))
						{
						  for (k = i + j;
						       k !=
						       statusFileRanges
						       [statusFileRangesPos]
						       [statusFileRangesSize];
						       k++)
						    {
						      statusFileMemAlloc[i] =
							statusFileMemAlloc[k];
						      i++;
						    }
						  j +=
						    statusFileRanges
						    [statusFileRangesPos]
						    [statusFileRangesSize +
						     1] -
						    statusFileRanges
						    [statusFileRangesPos]
						    [statusFileRangesSize];
						  statusFileRangesSize += 2;
						  if (statusFileRangesSize ==
						      1 <<
						      (statusFileRangesPos +
						       1))
						    {
						      statusFileRangesSize =
							0;
						      statusFileRangesPos++;
						    }
						}
					    }
					}
				    }
				  else
				    {
				      if (!
					  (statusFileAddLocPos ==
					   statusFileAddLocPosMax
					   && statusFileAddLocSize ==
					   statusFileAddLocSizeMax))
					{
					  for (k = i + j;
					       k !=
					       statusFileAddLoc
					       [statusFileAddLocPos]
					       [statusFileAddLocSize]; k++)
					    {
					      statusFileMemAlloc[i] =
						statusFileMemAlloc[k];
					      i++;
					    }
					}
				    }
				}
			    }
			  else
			    {
			      n =
				statusFileAddSize[statusFileAddLocPos]
				[statusFileAddLocSize];
			      statusFileAddStrSize += n;
			      if (statusFileAddStrSize <=
				  1 << statusFileAddStrPos)
				{
				  memcpy (statusFileMemAlloc + i,
					  statusFileAddStr
					  [statusFileAddStrPos] +
					  statusFileAddStrSize - n, n);
				}
			      else
				{
				  memcpy (statusFileMemAlloc + i,
					  statusFileAddStr
					  [statusFileAddStrPos] +
					  statusFileAddStrSize - n,
					  (1 << statusFileAddStrPos) -
					  statusFileAddStrSize + n);
				  statusFileAddStrSize =
				    (1 << statusFileAddStrPos) -
				    statusFileAddStrSize + n;
				  n -= statusFileAddStrSize;
				  statusFileAddStrSize += i;
				  while (n != 0)
				    {
				      statusFileAddStrPos++;
				      if (n > 1 << statusFileAddStrPos)
					{
					  memcpy (statusFileMemAlloc +
						  statusFileAddStrSize,
						  statusFileAddStr
						  [statusFileAddStrPos],
						  1 << statusFileAddStrPos);
					}
				      else
					{
					  memcpy (statusFileMemAlloc +
						  statusFileAddStrSize,
						  statusFileAddStr
						  [statusFileAddStrPos], n);
					  statusFileAddStrSize =
					    n - (1 << statusFileAddStrPos);
					  n = 1 << statusFileAddStrPos;
					}
				      statusFileAddStrSize +=
					1 << statusFileAddStrPos;
				      n -= 1 << statusFileAddStrPos;
				    }
				}
			      if (statusFileAddStrSize ==
				  1 << statusFileAddStrPos)
				{
				  statusFileAddStrPos++;
				  statusFileAddStrSize = 0;
				}
			      i +=
				statusFileAddSize[statusFileAddLocPos]
				[statusFileAddLocSize];
			      j -=
				statusFileAddSize[statusFileAddLocPos]
				[statusFileAddLocSize];
			      statusFileAddLocSize++;
			      if (statusFileAddLocSize ==
				  1 << statusFileAddLocPos)
				{
				  statusFileAddLocSize = 0;
				  statusFileAddLocPos++;
				}
			      if (statusFileAddLocPos ==
				  statusFileAddLocPosMax
				  && statusFileAddLocSize ==
				  statusFileAddLocSizeMax)
				{
				  while (!
					 (statusFileRangesPos ==
					  statusFileRangesPosMax
					  && statusFileRangesSize ==
					  statusFileRangesSizeMax))
				    {
				      for (k = i + j;
					   k !=
					   statusFileRanges
					   [statusFileRangesPos]
					   [statusFileRangesSize]; k++)
					{
					  statusFileMemAlloc[i] =
					    statusFileMemAlloc[k];
					  i++;
					}
				      j +=
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize + 1] -
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize];
				      statusFileRangesSize += 2;
				      if (statusFileRangesSize ==
					  1 << (statusFileRangesPos + 1))
					{
					  statusFileRangesSize = 0;
					  statusFileRangesPos++;
					}
				    }
				}
			      if (j
				  && !(statusFileRangesPos ==
				       statusFileRangesPosMax
				       && statusFileRangesSize ==
				       statusFileRangesSizeMax
				       && statusFileAddLocPos ==
				       statusFileAddLocPosMax
				       && statusFileAddLocSize ==
				       statusFileAddLocSizeMax))
				{
				  if (!
				      (statusFileRangesPos ==
				       statusFileRangesPosMax
				       && statusFileRangesSize ==
				       statusFileRangesSizeMax)
				      &&
				      statusFileRanges[statusFileRangesPos]
				      [statusFileRangesSize + 1] <
				      statusFileAddLoc[statusFileAddLocPos]
				      [statusFileAddLocSize])
				    {
				      for (k = i + j;
					   k !=
					   statusFileRanges
					   [statusFileRangesPos]
					   [statusFileRangesSize]; k++)
					{
					  statusFileMemAlloc[i] =
					    statusFileMemAlloc[k];
					  i++;
					}
				    }
				  else
				    {
				      if (!
					  (statusFileAddLocPos ==
					   statusFileAddLocPosMax
					   && statusFileAddLocSize ==
					   statusFileAddLocSizeMax))
					{
					  for (k = i + j;
					       k !=
					       statusFileAddLoc
					       [statusFileAddLocPos]
					       [statusFileAddLocSize]; k++)
					    {
					      statusFileMemAlloc[i] =
						statusFileMemAlloc[k];
					      i++;
					    }
					}
				    }
				  i--;
				}
			    }
			}
		      else
			{
			  i =
			    statusFileRanges[statusFileRangesPos]
			    [statusFileRangesSize] - j;
			  if (!
			      (statusFileAddLocPos == statusFileAddLocPosMax
			       && statusFileAddLocSize ==
			       statusFileAddLocSizeMax))
			    {
			      j +=
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize + 1] -
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize];
			    }
			  if (!
			      (statusFileRangesPos == statusFileRangesPosMax
			       && statusFileRangesSize ==
			       statusFileRangesSizeMax)
			      && !(statusFileAddLocPos ==
				   statusFileAddLocPosMax
				   && statusFileAddLocSize ==
				   statusFileAddLocSizeMax)
			      &&
			      statusFileRanges[statusFileRangesPos]
			      [statusFileRangesSize + 1] <
			      statusFileAddLoc[statusFileAddLocPos]
			      [statusFileAddLocSize])
			    {
			      statusFileRangesSize += 2;
			      if (statusFileRangesSize ==
				  1 << (statusFileRangesPos + 1))
				{
				  statusFileRangesSize = 0;
				  statusFileRangesPos++;
				}
			    }
			  while (!
				 (statusFileRangesPos ==
				  statusFileRangesPosMax
				  && statusFileRangesSize ==
				  statusFileRangesSizeMax)
				 && !(statusFileAddLocPos ==
				      statusFileAddLocPosMax
				      && statusFileAddLocSize ==
				      statusFileAddLocSizeMax)
				 &&
				 statusFileRanges[statusFileRangesPos]
				 [statusFileRangesSize + 1] <
				 statusFileAddLoc[statusFileAddLocPos]
				 [statusFileAddLocSize])
			    {
			      if (!
				  (statusFileRangesPos ==
				   statusFileRangesPosMax
				   && statusFileRangesSize ==
				   statusFileRangesSizeMax))
				{
				  for (k = i + j;
				       k !=
				       statusFileRanges[statusFileRangesPos]
				       [statusFileRangesSize]; k++)
				    {
				      statusFileMemAlloc[i] =
					statusFileMemAlloc[k];
				      i++;
				    }
				  j +=
				    statusFileRanges[statusFileRangesPos]
				    [statusFileRangesSize + 1] -
				    statusFileRanges[statusFileRangesPos]
				    [statusFileRangesSize];
				  statusFileRangesSize += 2;
				  if (statusFileRangesSize ==
				      1 << (statusFileRangesPos + 1))
				    {
				      statusFileRangesSize = 0;
				      statusFileRangesPos++;
				    }
				}
			    }
			  if (!
			      (statusFileAddLocPos == statusFileAddLocPosMax
			       && statusFileAddLocSize ==
			       statusFileAddLocSizeMax))
			    {
			      for (k = i + j;
				   k !=
				   statusFileAddLoc[statusFileAddLocPos]
				   [statusFileAddLocSize]; k++)
				{
				  statusFileMemAlloc[i] =
				    statusFileMemAlloc[k];
				  i++;
				}
			      i--;
			    }
			}
		      k = 0;
		      if (statusFileAddLocPos == statusFileAddLocPosMax
			  && statusFileAddLocSize == statusFileAddLocSizeMax)
			{
			  k = i;
			  i = statusFileSize - 2;
			}
		    }
		}
	      else
		{
		  for (s = i = 0; i != statusFileSize - 1; i++)
		    {
		      if (!
			  (statusFileAddLocPos == statusFileAddLocPosMax
			   && statusFileAddLocSize == statusFileAddLocSizeMax)
			  &&
			  statusFileAddLoc[statusFileAddLocPos]
			  [statusFileAddLocSize] <
			  statusFileRanges[statusFileRangesPos]
			  [statusFileRangesSize])
			{
			  if (j < statusFileAddStRSize[s])
			    {
			      i = j;
			      j = statusFileAddStRSize[s] - i;
			      n = statusFileAddStRSize[s];
			      remaining =
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize + 1] -
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize];
			      while (j > remaining)
				{
				  j -= remaining;
				  statusFileRangesSize += 2;
				  if (statusFileRangesSize ==
				      1 << (statusFileRangesPos + 1))
				    {
				      statusFileRangesSize = 0;
				      statusFileRangesPos++;
				    }
				  remaining =
				    statusFileRanges[statusFileRangesPos]
				    [statusFileRangesSize + 1] -
				    statusFileRanges[statusFileRangesPos]
				    [statusFileRangesSize];
				}
			      s++;
			      statusFileAddLocSize++;
			      if (statusFileAddLocSize ==
				  1 << statusFileAddLocPos)
				{
				  statusFileAddLocSize = 0;
				  statusFileAddLocPos++;
				}
			      l = 0;
			      for (k = 1;
				   !(statusFileAddLocPos ==
				     statusFileAddLocPosMax
				     && statusFileAddLocSize ==
				     statusFileAddLocSizeMax)
				   && (j > remaining
				       ||
				       statusFileAddLoc[statusFileAddLocPos]
				       [statusFileAddLocSize] <
				       statusFileRanges[statusFileRangesPos]
				       [statusFileRangesSize + 1]); k++)
				{
				  j += statusFileAddStRSize[s];
				  n = statusFileAddStRSize[s];
				  while (j > remaining)
				    {
				      j -= remaining;
				      statusFileRangesSize += 2;
				      if (statusFileRangesSize ==
					  1 << (statusFileRangesPos + 1))
					{
					  statusFileRangesSize = 0;
					  statusFileRangesPos++;
					}
				      remaining =
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize + 1] -
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize];
				    }
				  s++;
				  statusFileAddLocSize++;
				  if (statusFileAddLocSize ==
				      1 << statusFileAddLocPos)
				    {
				      statusFileAddLocSize = 0;
				      statusFileAddLocPos++;
				    }
				}
			      j = remaining - j;
			      o = statusFileRangesPos;
			      p = statusFileRangesSize;
			      q = statusFileAddLocPos;
			      r = statusFileAddLocSize;
			      t = s;
			      if (statusFileAddLocSize == 0)
				{
				  statusFileAddLocPos--;
				  statusFileAddLocSize =
				    1 << statusFileAddLocPos;
				}
			      statusFileAddLocSize--;
			      m =
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize + 1] - 1 - j;
			      l =
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize] - 1;
			      remaining = 0;
			      if (!
				  (statusFileRangesPos | statusFileRangesSize
				   | remaining)
				  &&
				  statusFileAddLoc[statusFileAddLocPos]
				  [statusFileAddLocSize] <
				  statusFileRanges[statusFileRangesPos]
				  [statusFileRangesSize + 1])
				{
				  remaining--;
				}
			      if ((statusFileRangesPos | statusFileRangesSize)
				  &&
				  statusFileAddLoc[statusFileAddLocPos]
				  [statusFileAddLocSize] <
				  statusFileRanges[statusFileRangesPos]
				  [statusFileRangesSize + 1])
				{
				  if (statusFileRangesPos)
				    {
				      if (!statusFileRangesSize)
					{
					  statusFileRangesPos--;
					  statusFileRangesSize =
					    1 << (statusFileRangesPos + 1);
					}
				    }
				  else if (!statusFileRangesSize)
				    {
				      statusFileRangesSize = 2;
				    }
				  statusFileRangesSize -= 2;
				}
			      for (; k; k--)
				{
				  while ((statusFileRangesPos |
					  statusFileRangesSize)
					 &&
					 statusFileAddLoc[statusFileAddLocPos]
					 [statusFileAddLocSize] <
					 statusFileRanges[statusFileRangesPos]
					 [statusFileRangesSize + 1])
				    {
				      for (;
					   l !=
					   statusFileRanges
					   [statusFileRangesPos]
					   [statusFileRangesSize + 1] - 1;
					   l--)
					{
					  statusFileMemAlloc[m] =
					    statusFileMemAlloc[l];
					  m--;
					}
				      l =
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize] - 1;
				      if (statusFileRangesPos)
					{
					  if (!statusFileRangesSize)
					    {
					      statusFileRangesPos--;
					      statusFileRangesSize =
						1 << (statusFileRangesPos +
						      1);
					    }
					}
				      else if (!statusFileRangesSize)
					{
					  statusFileRangesSize = 2;
					}
				      statusFileRangesSize -= 2;
				    }
				  if (!
				      (statusFileRangesPos |
				       statusFileRangesSize | remaining)
				      &&
				      statusFileAddLoc[statusFileAddLocPos]
				      [statusFileAddLocSize] <
				      statusFileRanges[statusFileRangesPos]
				      [statusFileRangesSize + 1])
				    {
				      for (;
					   l !=
					   statusFileRanges
					   [statusFileRangesPos]
					   [statusFileRangesSize + 1] - 1;
					   l--)
					{
					  statusFileMemAlloc[m] =
					    statusFileMemAlloc[l];
					  m--;
					}
				      l =
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize] - 1;
				      remaining--;
				    }
				  for (;
				       l !=
				       statusFileAddLoc[statusFileAddLocPos]
				       [statusFileAddLocSize] - 1; l--)
				    {
				      statusFileMemAlloc[m] =
					statusFileMemAlloc[l];
				      m--;
				    }
				  m++;
				  s--;
				  n = statusFileAddStRSize[s];
				  statusFileAddStrPos =
				    offset[statusFileAddLocPos]
				    [statusFileAddLocSize];
				  statusFileAddStrSize = 0;
				  statusFileAddStrSize--;
				  statusFileAddStrSize >>= 6;
				  statusFileAddStrSize =
				    statusFileAddStrPos &
				    statusFileAddStrSize;
				  statusFileAddStrPos >>=
				    (sizeof
				     (offset[statusFileAddLocPos]
				      [statusFileAddLocSize]) * 8 - 6);
				  m -= n;
				  statusFileAddStrSize += n;
				  if (statusFileAddStrSize <=
				      1 << statusFileAddStrPos)
				    {
				      memcpy (statusFileMemAlloc + m,
					      statusFileAddStr
					      [statusFileAddStrPos] +
					      statusFileAddStrSize - n, n);
				    }
				  else
				    {
				      memcpy (statusFileMemAlloc + m,
					      statusFileAddStr
					      [statusFileAddStrPos] +
					      statusFileAddStrSize - n,
					      (1 << statusFileAddStrPos) -
					      statusFileAddStrSize + n);
				      statusFileAddStrSize =
					(1 << statusFileAddStrPos) -
					statusFileAddStrSize + n;
				      n -= statusFileAddStrSize;
				      statusFileAddStrSize += m;
				      while (n != 0)
					{
					  statusFileAddStrPos++;
					  if (n > 1 << statusFileAddStrPos)
					    {
					      memcpy (statusFileMemAlloc +
						      statusFileAddStrSize,
						      statusFileAddStr
						      [statusFileAddStrPos],
						      1 <<
						      statusFileAddStrPos);
					    }
					  else
					    {
					      memcpy (statusFileMemAlloc +
						      statusFileAddStrSize,
						      statusFileAddStr
						      [statusFileAddStrPos],
						      n);
					      statusFileAddStrSize =
						n -
						(1 << statusFileAddStrPos);
					      n = 1 << statusFileAddStrPos;
					    }
					  statusFileAddStrSize +=
					    1 << statusFileAddStrPos;
					  n -= 1 << statusFileAddStrPos;
					}
				    }
				  m--;
				  if (statusFileAddLocSize == 0)
				    {
				      statusFileAddLocPos--;
				      statusFileAddLocSize =
					1 << statusFileAddLocPos;
				    }
				  statusFileAddLocSize--;
				}
			      statusFileRangesPos = o;
			      statusFileRangesSize = p;
			      statusFileAddLocPos = q;
			      statusFileAddLocSize = r;
			      s = t;
			      i =
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize + 1] - j;
			      statusFileRangesSize += 2;
			      if (statusFileRangesSize ==
				  1 << (statusFileRangesPos + 1))
				{
				  statusFileRangesSize = 0;
				  statusFileRangesPos++;
				}
			      if (j
				  && !(statusFileRangesPos ==
				       statusFileRangesPosMax
				       && statusFileRangesSize ==
				       statusFileRangesSizeMax
				       && statusFileAddLocPos ==
				       statusFileAddLocPosMax
				       && statusFileAddLocSize ==
				       statusFileAddLocSizeMax))
				{
				  if (!
				      (statusFileRangesPos ==
				       statusFileRangesPosMax
				       && statusFileRangesSize ==
				       statusFileRangesSizeMax)
				      &&
				      ((statusFileAddLocPos ==
					statusFileAddLocPosMax
					&& statusFileAddLocSize ==
					statusFileAddLocSizeMax)
				       ||
				       statusFileRanges[statusFileRangesPos]
				       [statusFileRangesSize + 1] <
				       statusFileAddLoc[statusFileAddLocPos]
				       [statusFileAddLocSize]))
				    {
				      if (!
					  (statusFileRangesPos ==
					   statusFileRangesPosMax
					   && statusFileRangesSize ==
					   statusFileRangesSizeMax))
					{
					  for (k = i + j;
					       k !=
					       statusFileRanges
					       [statusFileRangesPos]
					       [statusFileRangesSize]; k++)
					    {
					      statusFileMemAlloc[i] =
						statusFileMemAlloc[k];
					      i++;
					    }
					  if (statusFileAddLocPos ==
					      statusFileAddLocPosMax
					      && statusFileAddLocSize ==
					      statusFileAddLocSizeMax)
					    {
					      j +=
						statusFileRanges
						[statusFileRangesPos]
						[statusFileRangesSize + 1] -
						statusFileRanges
						[statusFileRangesPos]
						[statusFileRangesSize];
					      statusFileRangesSize += 2;
					      if (statusFileRangesSize ==
						  1 << (statusFileRangesPos +
							1))
						{
						  statusFileRangesSize = 0;
						  statusFileRangesPos++;
						}
					      while (!
						     (statusFileRangesPos ==
						      statusFileRangesPosMax
						      && statusFileRangesSize
						      ==
						      statusFileRangesSizeMax))
						{
						  for (k = i + j;
						       k !=
						       statusFileRanges
						       [statusFileRangesPos]
						       [statusFileRangesSize];
						       k++)
						    {
						      statusFileMemAlloc[i] =
							statusFileMemAlloc[k];
						      i++;
						    }
						  j +=
						    statusFileRanges
						    [statusFileRangesPos]
						    [statusFileRangesSize +
						     1] -
						    statusFileRanges
						    [statusFileRangesPos]
						    [statusFileRangesSize];
						  statusFileRangesSize += 2;
						  if (statusFileRangesSize ==
						      1 <<
						      (statusFileRangesPos +
						       1))
						    {
						      statusFileRangesSize =
							0;
						      statusFileRangesPos++;
						    }
						}
					    }
					}
				    }
				  else
				    {
				      if (!
					  (statusFileAddLocPos ==
					   statusFileAddLocPosMax
					   && statusFileAddLocSize ==
					   statusFileAddLocSizeMax))
					{
					  for (k = i + j;
					       k !=
					       statusFileAddLoc
					       [statusFileAddLocPos]
					       [statusFileAddLocSize]; k++)
					    {
					      statusFileMemAlloc[i] =
						statusFileMemAlloc[k];
					      i++;
					    }
					}
				    }
				}
			    }
			  else
			    {
			      n = statusFileAddStRSize[s];
			      s++;
			      statusFileAddStrPos =
				offset[statusFileAddLocPos]
				[statusFileAddLocSize];
			      statusFileAddStrSize = 0;
			      statusFileAddStrSize--;
			      statusFileAddStrSize >>= 6;
			      statusFileAddStrSize =
				statusFileAddStrPos & statusFileAddStrSize;
			      statusFileAddStrPos >>=
				(sizeof
				 (offset[statusFileAddLocPos]
				  [statusFileAddLocSize]) * 8 - 6);
			      statusFileAddStrSize += n;
			      if (statusFileAddStrSize <=
				  1 << statusFileAddStrPos)
				{
				  memcpy (statusFileMemAlloc + i,
					  statusFileAddStr
					  [statusFileAddStrPos] +
					  statusFileAddStrSize - n, n);
				}
			      else
				{
				  memcpy (statusFileMemAlloc + i,
					  statusFileAddStr
					  [statusFileAddStrPos] +
					  statusFileAddStrSize - n,
					  (1 << statusFileAddStrPos) -
					  statusFileAddStrSize + n);
				  statusFileAddStrSize =
				    (1 << statusFileAddStrPos) -
				    statusFileAddStrSize + n;
				  n -= statusFileAddStrSize;
				  statusFileAddStrSize += i;
				  while (n != 0)
				    {
				      statusFileAddStrPos++;
				      if (n > 1 << statusFileAddStrPos)
					{
					  memcpy (statusFileMemAlloc +
						  statusFileAddStrSize,
						  statusFileAddStr
						  [statusFileAddStrPos],
						  1 << statusFileAddStrPos);
					}
				      else
					{
					  memcpy (statusFileMemAlloc +
						  statusFileAddStrSize,
						  statusFileAddStr
						  [statusFileAddStrPos], n);
					  statusFileAddStrSize =
					    n - (1 << statusFileAddStrPos);
					  n = 1 << statusFileAddStrPos;
					}
				      statusFileAddStrSize +=
					1 << statusFileAddStrPos;
				      n -= 1 << statusFileAddStrPos;
				    }
				}
			      if (statusFileAddStrSize ==
				  1 << statusFileAddStrPos)
				{
				  statusFileAddStrPos++;
				  statusFileAddStrSize = 0;
				}
			      i += statusFileAddStRSize[s];
			      j -= statusFileAddStRSize[s];
			      statusFileAddLocSize++;
			      if (statusFileAddLocSize ==
				  1 << statusFileAddLocPos)
				{
				  statusFileAddLocSize = 0;
				  statusFileAddLocPos++;
				}
			      if (statusFileAddLocPos ==
				  statusFileAddLocPosMax
				  && statusFileAddLocSize ==
				  statusFileAddLocSizeMax)
				{
				  while (!
					 (statusFileRangesPos ==
					  statusFileRangesPosMax
					  && statusFileRangesSize ==
					  statusFileRangesSizeMax))
				    {
				      for (k = i + j;
					   k !=
					   statusFileRanges
					   [statusFileRangesPos]
					   [statusFileRangesSize]; k++)
					{
					  statusFileMemAlloc[i] =
					    statusFileMemAlloc[k];
					  i++;
					}
				      j +=
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize + 1] -
					statusFileRanges[statusFileRangesPos]
					[statusFileRangesSize];
				      statusFileRangesSize += 2;
				      if (statusFileRangesSize ==
					  1 << (statusFileRangesPos + 1))
					{
					  statusFileRangesSize = 0;
					  statusFileRangesPos++;
					}
				    }
				}
			      if (j
				  && !(statusFileRangesPos ==
				       statusFileRangesPosMax
				       && statusFileRangesSize ==
				       statusFileRangesSizeMax
				       && statusFileAddLocPos ==
				       statusFileAddLocPosMax
				       && statusFileAddLocSize ==
				       statusFileAddLocSizeMax))
				{
				  if (!
				      (statusFileRangesPos ==
				       statusFileRangesPosMax
				       && statusFileRangesSize ==
				       statusFileRangesSizeMax)
				      &&
				      statusFileRanges[statusFileRangesPos]
				      [statusFileRangesSize + 1] <
				      statusFileAddLoc[statusFileAddLocPos]
				      [statusFileAddLocSize])
				    {
				      for (k = i + j;
					   k !=
					   statusFileRanges
					   [statusFileRangesPos]
					   [statusFileRangesSize]; k++)
					{
					  statusFileMemAlloc[i] =
					    statusFileMemAlloc[k];
					  i++;
					}
				    }
				  else
				    {
				      if (!
					  (statusFileAddLocPos ==
					   statusFileAddLocPosMax
					   && statusFileAddLocSize ==
					   statusFileAddLocSizeMax))
					{
					  for (k = i + j;
					       k !=
					       statusFileAddLoc
					       [statusFileAddLocPos]
					       [statusFileAddLocSize]; k++)
					    {
					      statusFileMemAlloc[i] =
						statusFileMemAlloc[k];
					      i++;
					    }
					}
				    }
				  i--;
				}
			    }
			}
		      else
			{
			  i =
			    statusFileRanges[statusFileRangesPos]
			    [statusFileRangesSize] - j;
			  if (!
			      (statusFileAddLocPos == statusFileAddLocPosMax
			       && statusFileAddLocSize ==
			       statusFileAddLocSizeMax))
			    {
			      j +=
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize + 1] -
				statusFileRanges[statusFileRangesPos]
				[statusFileRangesSize];
			    }
			  if (!
			      (statusFileRangesPos == statusFileRangesPosMax
			       && statusFileRangesSize ==
			       statusFileRangesSizeMax)
			      && !(statusFileAddLocPos ==
				   statusFileAddLocPosMax
				   && statusFileAddLocSize ==
				   statusFileAddLocSizeMax)
			      &&
			      statusFileRanges[statusFileRangesPos]
			      [statusFileRangesSize + 1] <
			      statusFileAddLoc[statusFileAddLocPos]
			      [statusFileAddLocSize])
			    {
			      statusFileRangesSize += 2;
			      if (statusFileRangesSize ==
				  1 << (statusFileRangesPos + 1))
				{
				  statusFileRangesSize = 0;
				  statusFileRangesPos++;
				}
			    }
			  while (!
				 (statusFileRangesPos ==
				  statusFileRangesPosMax
				  && statusFileRangesSize ==
				  statusFileRangesSizeMax)
				 && !(statusFileAddLocPos ==
				      statusFileAddLocPosMax
				      && statusFileAddLocSize ==
				      statusFileAddLocSizeMax)
				 &&
				 statusFileRanges[statusFileRangesPos]
				 [statusFileRangesSize + 1] <
				 statusFileAddLoc[statusFileAddLocPos]
				 [statusFileAddLocSize])
			    {
			      if (!
				  (statusFileRangesPos ==
				   statusFileRangesPosMax
				   && statusFileRangesSize ==
				   statusFileRangesSizeMax))
				{
				  for (k = i + j;
				       k !=
				       statusFileRanges[statusFileRangesPos]
				       [statusFileRangesSize]; k++)
				    {
				      statusFileMemAlloc[i] =
					statusFileMemAlloc[k];
				      i++;
				    }
				  j +=
				    statusFileRanges[statusFileRangesPos]
				    [statusFileRangesSize + 1] -
				    statusFileRanges[statusFileRangesPos]
				    [statusFileRangesSize];
				  statusFileRangesSize += 2;
				  if (statusFileRangesSize ==
				      1 << (statusFileRangesPos + 1))
				    {
				      statusFileRangesSize = 0;
				      statusFileRangesPos++;
				    }
				}
			    }
			  if (!
			      (statusFileAddLocPos == statusFileAddLocPosMax
			       && statusFileAddLocSize ==
			       statusFileAddLocSizeMax))
			    {
			      for (k = i + j;
				   k !=
				   statusFileAddLoc[statusFileAddLocPos]
				   [statusFileAddLocSize]; k++)
				{
				  statusFileMemAlloc[i] =
				    statusFileMemAlloc[k];
				  i++;
				}
			      i--;
			    }
			}
		      k = 0;
		      if (statusFileAddLocPos == statusFileAddLocPosMax
			  && statusFileAddLocSize == statusFileAddLocSizeMax)
			{
			  k = i;
			  i = statusFileSize - 2;
			}
		    }
		}
	      if (!
		  (statusFileRangesPos == statusFileRangesPosMax
		   && statusFileRangesSize == statusFileRangesSizeMax))
		{
		  k =
		    statusFileRanges[statusFileRangesPos]
		    [statusFileRangesSize] + j;
		  j +=
		    statusFileRanges[statusFileRangesPos][statusFileRangesSize
							  + 1] -
		    statusFileRanges[statusFileRangesPos]
		    [statusFileRangesSize];
		}
	      for (i = k;
		   i != statusFileSize - 1
		   && !(statusFileRangesPos == statusFileRangesPosMax
			&& statusFileRangesSize == statusFileRangesSizeMax);
		   k = i)
		{
		  statusFileRangesSize += 2;
		  if (statusFileRangesSize == 1 << (statusFileRangesPos + 1))
		    {
		      statusFileRangesSize = 0;
		      statusFileRangesPos++;
		    }
		  if (!
		      (statusFileRangesPos == statusFileRangesPosMax
		       && statusFileRangesSize == statusFileRangesSizeMax))
		    {
		      for (k = i + j;
			   k !=
			   statusFileRanges[statusFileRangesPos]
			   [statusFileRangesSize]; k++)
			{
			  statusFileMemAlloc[i] = statusFileMemAlloc[k];
			  i++;
			}
		      j +=
			statusFileRanges[statusFileRangesPos]
			[statusFileRangesSize + 1] -
			statusFileRanges[statusFileRangesPos]
			[statusFileRangesSize];
		    }
		}
	      if (k)
		{
		  if (statusFileRangesSize | statusFileRangesPos)
		    {
		      if (statusFileRangesPos)
			{
			  if (!statusFileRangesSize)
			    {
			      statusFileRangesPos--;
			      statusFileRangesSize =
				1 << (statusFileRangesPos + 1);
			    }
			}
		      else if (!statusFileRangesSize)
			{
			  statusFileRangesSize = 2;
			}
		      statusFileRangesSize -= 2;
		    }
		  i = k;
		  for (k = i + j; i != newstatusFileSize; i++)
		    {
		      statusFileMemAlloc[i] = statusFileMemAlloc[k];
		      k++;
		    }
		}
	      if (newstatusFileSize < statusFileSize)
		{
		  munmap (statusFileMemAlloc, statusFileSize);
		  statusFileSize = newstatusFileSize;
		  ftruncate (statusFile, statusFileSize);
		  statusFileMemAlloc =
		    mmap (0, statusFileSize, PROT_READ | PROT_WRITE,
			  MAP_SHARED, statusFile, 0);
		}
	      if (deloccured1)
		{
		  if (statusFileRequestSorted)
		    {
		      n =
			statusFileAddSize[statusFileAddLocPos]
			[statusFileAddLocSize];
		      i = statusFileSize - n;
		    }
		  else
		    {
		      n = statusFileAddStRSize[s];
		      i = statusFileSize - n;
		      s++;
		      statusFileAddStrPos =
			offset[statusFileAddLocPos][statusFileAddLocSize];
		      statusFileAddStrSize = 0;
		      statusFileAddStrSize--;
		      statusFileAddStrSize >>= 6;
		      statusFileAddStrSize =
			statusFileAddStrPos & statusFileAddStrSize;
		      statusFileAddStrPos >>=
			(sizeof
			 (offset[statusFileAddLocPos][statusFileAddLocSize]) *
			 8 - 6);
		    }
		  statusFileAddStrSize += n;
		  if (statusFileAddStrSize <= 1 << statusFileAddStrPos)
		    {
		      memcpy (statusFileMemAlloc + i,
			      statusFileAddStr[statusFileAddStrPos] +
			      statusFileAddStrSize - n, n);
		    }
		  else
		    {
		      memcpy (statusFileMemAlloc + i,
			      statusFileAddStr[statusFileAddStrPos] +
			      statusFileAddStrSize - n,
			      (1 << statusFileAddStrPos) -
			      statusFileAddStrSize + n);
		      statusFileAddStrSize =
			(1 << statusFileAddStrPos) - statusFileAddStrSize + n;
		      n -= statusFileAddStrSize;
		      statusFileAddStrSize += i;
		      while (n != 0)
			{
			  statusFileAddStrPos++;
			  if (n > 1 << statusFileAddStrPos)
			    {
			      memcpy (statusFileMemAlloc +
				      statusFileAddStrSize,
				      statusFileAddStr[statusFileAddStrPos],
				      1 << statusFileAddStrPos);
			    }
			  else
			    {
			      memcpy (statusFileMemAlloc +
				      statusFileAddStrSize,
				      statusFileAddStr[statusFileAddStrPos],
				      n);
			      statusFileAddStrSize =
				n - (1 << statusFileAddStrPos);
			      n = 1 << statusFileAddStrPos;
			    }
			  statusFileAddStrSize += 1 << statusFileAddStrPos;
			  n -= 1 << statusFileAddStrPos;
			}
		    }
		  if (statusFileAddStrSize == 1 << statusFileAddStrPos)
		    {
		      statusFileAddStrPos++;
		      statusFileAddStrSize = 0;
		    }
		  i +=
		    statusFileAddSize[statusFileAddLocPos]
		    [statusFileAddLocSize];
		  j -=
		    statusFileAddSize[statusFileAddLocPos]
		    [statusFileAddLocSize];
		  statusFileAddLocSize++;
		  if (statusFileAddLocSize == 1 << statusFileAddLocPos)
		    {
		      statusFileAddLocSize = 0;
		      statusFileAddLocPos++;
		    }
		}
	    }
	}
    }
  return 0;
}
