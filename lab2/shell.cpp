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

void run_cmd(std::vector<std::string>);
void run_redi_cmd(std::vector<std::string>);
void run_pipe_cmd(std::string);
void sighandler(int);

// used in handling ctrl c
sigjmp_buf env;

// a sign for background cmd
bool is_background_cmd;

// store bg_pid
std::vector<pid_t> bg_pid;

std::vector<std::string> split(std::string s, const std::string &delimiter);

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


    //handle ctrl d
    //sth. about peek() function:
    //cin.peek()的返回值是一个char型的字符，其返回值是指针指向的当前字符，但它只是观测
    //指针停留在当前位置并不后移；如果要访问的字符是文件结束符，则函数值是EOF(-1)
    if(std::cin.peek() == EOF)
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
        chdir("/home");
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
      // wait cmd version1
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

// 经典的 cpp string split 实现
// https://stackoverflow.com/a/14266139/11691878
std::vector<std::string> split(std::string s, const std::string &delimiter)
{
  std::vector<std::string> res;
  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delimiter)) != std::string::npos)
  {
    token = s.substr(0, pos);
    res.push_back(token);
    s = s.substr(pos + delimiter.length());
  }
  res.push_back(s);
  return res;
}

void run_cmd(std::vector<std::string> args)
{
  // run command that doesn't have redirection/pipe(i.e. > >> < |)

  // std::vector<std::string> 转 char **
  char *arg_ptrs[args.size() + 1];
  for (__SIZE_TYPE__ i = 0; i < args.size(); i++)
  {
    arg_ptrs[i] = &args[i][0];
  }
  // exec p 系列的 argv 需要以 nullptr 结尾
  arg_ptrs[args.size()] = nullptr;
  execvp(args[0].c_str(), arg_ptrs);
}

void run_redi_cmd(std::vector<std::string> args)
{
  // run redirection command
  int index = 0;
  std::vector<std::string> cmd;
  while ((__SIZE_TYPE__)index < args.size())
  {
    // encounter some problems here
    //"cat < t1.txt > t2.txt" is different from zsh.
    cmd.push_back(args[index]);
    if (args[index] == ">")
    {
      cmd.pop_back();
      pid_t pid = fork();
      if (pid == 0)
      {
        int fd = open(args[index + 1].c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        dup2(fd, 1);
        run_cmd(cmd);
        cmd.clear();
        close(fd);
      }
      waitpid(pid, NULL, 0);
    }
    else if (args[index] == ">>")
    {
      cmd.pop_back();
      pid_t pid = fork();
      if (pid == 0)
      {
        int fd = open(args[index + 1].c_str(), O_APPEND | O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        dup2(fd, 1);
        run_cmd(cmd);
        cmd.clear();
        close(fd);
      }
      waitpid(pid, NULL, 0);
    }
    else if (args[index] == "<")
    {
      cmd.pop_back();
      pid_t pid = fork();
      if (pid == 0)
      {
        int fd = open(args[index + 1].c_str(), O_RDONLY);
        dup2(fd, 0);
        run_cmd(cmd);
        cmd.clear();
        close(fd);
      }
      waitpid(pid, NULL, 0);
    }
    index++;
  }
  index = 0;
  int count = 0;
  while ((__SIZE_TYPE__)index < args.size())
  {
    if (args[index] == ">" || args[index] == ">>" || args[index] == "<")
    {
      count++;
    }
    index++;
  }
  if (count == 0)
    // there is no redirection cmd
    run_cmd(args);

  return;
}

void run_pipe_cmd(std::string cmd)
{
  // split the cmd with " | ", pay attention to space.
  // In fact, I just let "|" have one space before "|" and after "|"
  std::vector<std::string> pipe_args = split(cmd, " | ");

  // note that cmd may have "&"
  // so I need to judge if there is "&"
  // if it does, I will pop it

  if (pipe_args.size() == 1)
  {
    // there is no "|"
    std::vector<std::string> args = split(cmd, " ");
    if (args[args.size() - 1] == "&")
      args.pop_back();
    run_redi_cmd(args);
  }
  else if (pipe_args.size() == 2)
  {
    // single "|"
    std::vector<std::string> args_left = split(pipe_args[0], " ");
    if (args_left[args_left.size() - 1] == "&")
      args_left.pop_back();
    std::vector<std::string> args_right = split(pipe_args[1], " ");
    if (args_right[args_right.size() - 1] == "&")
      args_right.pop_back();
    int fd[2];
    pipe(fd);
    pid_t pid1 = fork();
    if (pid1 == 0)
    {
      close(fd[0]);
      dup2(fd[1], 1);
      run_redi_cmd(args_left);
      close(fd[1]);
    }
    // father process
    pid_t pid2 = fork();
    if (pid2 == 0)
    {
      close(fd[1]);
      dup2(fd[0], 0);
      run_redi_cmd(args_right);
      close(fd[0]);
    }
    close(fd[0]);
    close(fd[1]);
    wait(NULL);
  }
  else
  {
    int read_end = 0;
    for (__SIZE_TYPE__ i = 0; i < pipe_args.size(); i++)
    {
      int fd[2];
      // the number of "|" is pipe_args.size() - 1
      if (i < pipe_args.size() - 1)
      {
        pipe(fd);
      }
      pid_t pid = fork();
      if (pid == 0)
      {
        dup2(read_end, 0);
        if (i < pipe_args.size() - 1)
        {
          dup2(fd[1], 1);
        }
        std::vector<std::string> args = split(pipe_args[i], " ");
        if (args[args.size() - 1] == "&")
          args.pop_back();
        run_redi_cmd(args);
      }
      else
      {
        close(fd[1]);
        if (i > 0)
          close(read_end);
        read_end = fd[0];
      }
    }
    wait(NULL);
  }
}

void sighandler(int sig)
{
  if (sig == SIGINT)
  {
    std::cout << std::endl;
    // https://stackoverflow.com/questions/16828378/readline-get-a-new-prompt-on-sigint
    siglongjmp(env, 1);
  }
}
