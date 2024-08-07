#define COLORS

#include "headers/exec.h"
#include "headers/escape.h"

#if _WIN32

BOOL FindExecutableInPath(LPCSTR executable, LPCSTR *path_found) {
  LPCSTR env_var = getenv("PATH");
  if (env_var == NULL)
    fprintf(stderr,
            RED "!%s Failed to find path variable, is your windows pc ok?",
            CLEAR) return FALSE;

  LPCSTR path = env_var;
  do {
    size_t len = strlen(path);
    if (len > 0 && path[len - 1] == ';') {
      path[len - 1] = '\0'; // Remove trailing semicolon
    }
    if (_stricmp(executable, path) == 0) {
      *path_found = path;
      return TRUE;
    }
    path += len + 1; // Move to next directory in PATH
  } while (*path);

  return FALSE;
}

#endif

int execute(char *executable, char **argv, int status) {

#if __linux__
  pid_t pid = fork();

  if (pid < 0) {
    perror("failed to fork");
  } else if (pid == 0) {
    if (execvp(executable, argv) == -1) {
      if (errno == ENOENT) {
        fprintf(stderr, RED "!%s %s: Command not found\n", CLEAR, executable);
      } else {
        perror("Failed to exec to childprocess. (execvp)");
      }
      return -1;
    }
  } else {
    wait(&status);
  }

#elif _WIN32

  LPCSTR win_executable = (LPCSTR)executable;
  LPCSTR path_found = {0};

  if (find_executable_in_path(win_executable, &path_found)) {
    STARTUPINFO start_i;
    PROCESS_INFORMATION proc_i;

    ZeroMemory(&start_i, sizeof(start_i));
    start_i.cb = sizeof(start_i);
    ZeroMemory(&proc_i, sizeof(proc_i));

    LPSTR full_path = malloc(strlen(path_found) + strlen(win_executable) + 2);
    strlcpy(full_path, path_found, strlen(full_path));
    strlcat(full_path, "\\", 2);
    strlcat(full_path, win_executable, strlen(win_executable));

    if (!CreateProcess(0, full_path, 0, 0, FALSE, 0, 0, 0, &start_i, &proc_i)) {
      fprintf(stderr, RED "!%s CreateProcess failed (%d).\n", CLEAR,
              GetLastError());
      return -1;
    }

    CloseHandle(proc_i.hProcess);
    CloseHandle(proc_i.hThread);
    free(full_path);
  } else {
    // Signify nonexistent executable.
    return 1;
  }

#endif
  return 0;
}
