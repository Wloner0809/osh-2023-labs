// IO
#include <iostream>
// std::string
#include <string>
// std::vector
#include <vector>
// std::string 转 int
#include <sstream>
// PATH_MAX 等常量
#include <climits>
// POSIX API
#include <unistd.h>
// wait
#include <sys/wait.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <signal.h>

#include <setjmp.h>

#include <algorithm>

#include "shell.h"

int main()
{
  // 不同步 iostream 和 cstdio 的 buffer
  std::ios::sync_with_stdio(false);

  signal(SIGINT, sighandler);

  // 用来存储读入的一行命令
  std::string cmd;
  while (true)
  {

    // 返回值：若直接调用则返回0，若从siglongjmp()调用返回则返回非0值
    while (sigsetjmp(env, 1) != 0)
      ;

    // 打印提示符
    std::cout << "# ";

    // handle ctrl d
    // sth. about peek() function:
    // cin.peek()的返回值是一个char型的字符，其返回值是指针指向的当前字符，但它只是观测
    // 指针停留在当前位置并不后移；如果要访问的字符是文件结束符，则函数值是EOF(-1)
    if (std::cin.peek() == EOF)
      exit(0);

    // 读入一行。std::getline 结果不包含换行符。
    std::getline(std::cin, cmd);

    // 按空格分割命令为单词
    std::vector<std::string> args = split(cmd, " ");

    // cmd is background or not
    if (args[args.size() - 1] == "&")
    {
      is_background_cmd = true;
      args.pop_back();
    }
    else
      is_background_cmd = false;

    // 没有可处理的命令
    if (args.empty())
    {
      continue;
    }

    // 退出
    if (args[0] == "exit")
    {
      if (args.size() <= 1)
      {
        return 0;
      }

      // std::string 转 int
      std::stringstream code_stream(args[1]);
      int code = 0;
      code_stream >> code;

      // 转换失败
      if (!code_stream.eof() || code_stream.fail())
      {
        std::cout << "Invalid exit code\n";
        continue;
      }

      return code;
    }
    // pwd and cd are built-in cmd.
    if (args[0] == "pwd")
    {
      std::string path;
      path.resize(PATH_MAX);
      getcwd(&path[0], path.size());
      std::cout << path << std::endl;
      continue;
    }

    if (args[0] == "cd")
    {
      if (args.size() <= 1)
        chdir(getenv("HOME"));
      else
        chdir(args[1].c_str());
      std::string path;
      path.resize(PATH_MAX);
      getcwd(&path[0], path.size());
      std::cout << path << std::endl;
      continue;
    }

    if (args[0] == "wait")
    {
      for (__SIZE_TYPE__ i = 0; i < bg_pid.size(); i++)
      {
        waitpid(bg_pid[i], NULL, 0);
      }
      for (__SIZE_TYPE__ i = 0; i < bg_pid.size(); i++)
      {
        bg_pid.pop_back();
      }
      continue;
    }

    if (args[0] == "echo")
    {
      // support echo $SHELL cmd
      if (args[1] == "$SHELL")
      {
        std::cout << getenv("SHELL") << "\n";
        continue;
      }
      // support echo ~root cmd
      else if (args[1] == "~root")
      {
        std::cout << "/root\n";
        continue;
      }
    }

    // 处理外部命令
    pid_t pid = fork();

    // bg_pid is used in wait cmd
    if (is_background_cmd)
    {
      bg_pid.push_back(pid);
    }

    bool sign = true;
    pid_t pgid = 0;

    if (pid == 0)
    {
      // 这里只有子进程才会进入
      // std::cout << "child process\n";

      if (sign)
        pgid = getpid();
      setpgid(pid, pgid);

      // std::cout << pgid << "\n";
      // std::cout << getpgid(pid) << "\n";
      // std::cout << getpgid(pgid) << "\n";

      run_pipe_cmd(cmd);
      // 所以这里直接报错
      exit(255);
    }

    // 这里只有父进程（原进程）才会进入
    // std::cout << "father process\n";

    if (sign)
      pgid = pid;
    setpgid(pid, pgid);

    // std::cout << getpgid(pid) << "\n";
    // std::cout << getpid() << "\n";
    // std::cout << getppid() << "\n";
    // std::cout << getpgid(getppid()) << "\n";

    if (sign)
    {
      tcsetpgrp(STDIN_FILENO, pgid);
      kill(pid, SIGCONT);
      sign = false;
    }

    // std::cout << getpgid(pid) << "\n";

    if (is_background_cmd)
    {
      // WNOHANG option
      //如果pid指定的子进程没有结束，则waitpid()函数立即返回0，
      //而不是阻塞在这个函数上等待；如果结束了，则返回该子进程的进程号
      waitpid(pid, NULL, WNOHANG);
    }
    else
      wait(nullptr);

    // recover the old foreground process
    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, getpgid(getpid()));
    signal(SIGTTOU, SIG_DFL);
  }
}
