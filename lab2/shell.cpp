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

void run_cmd(std::vector<std::string>);
void run_redi_cmd(std::vector<std::string>);
void run_pipe_cmd(std::vector<std::string>);

std::vector<std::string> split(std::string s, const std::string &delimiter);

int main()
{
  // 不同步 iostream 和 cstdio 的 buffer
  std::ios::sync_with_stdio(false);

  // 用来存储读入的一行命令
  std::string cmd;
  while (true)
  {
    // 打印提示符
    std::cout << "# ";

    // 读入一行。std::getline 结果不包含换行符。
    std::getline(std::cin, cmd);

    // 按空格分割命令为单词
    std::vector<std::string> args = split(cmd, " ");

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

    // 处理外部命令
    pid_t pid = fork();

    // // std::vector<std::string> 转 char **
    // char *arg_ptrs[args.size() + 1];
    // for (auto i = 0; i < args.size(); i++)
    // {
    //   arg_ptrs[i] = &args[i][0];
    // }
    // // exec p 系列的 argv 需要以 nullptr 结尾
    // arg_ptrs[args.size()] = nullptr;

    if (pid == 0)
    {
      // 这里只有子进程才会进入
      // execvp 会完全更换子进程接下来的代码，所以正常情况下 execvp 之后这里的代码就没意义了
      // 如果 execvp 之后的代码被运行了，那就是 execvp 出问题了
      // execvp(args[0].c_str(), arg_ptrs);
      run_pipe_cmd(args);
      // 所以这里直接报错
      exit(255);
    }

    // 这里只有父进程（原进程）才会进入
    int ret = wait(nullptr);
    if (ret < 0)
    {
      std::cout << "wait failed";
    }
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
    cmd.push_back(args[index]);
    if (args[index] == ">")
    {
      cmd.pop_back();
      pid_t pid = fork();
      if (pid == 0)
      {
        int fd = open(args[index + 1].c_str(), O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        dup2(fd, 1);
        run_cmd(cmd);
        cmd.clear();
        close(fd);
      }
      waitpid(pid, NULL, 0);
      index = index + 2;
    }
    else if (args[index] == ">>")
    {
      cmd.pop_back();
      pid_t pid = fork();
      if (pid == 0)
      {
        int fd = open(args[index + 1].c_str(), O_APPEND | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        dup2(fd, 1);
        run_cmd(cmd);
        cmd.clear();
        close(fd);
      }
      waitpid(pid, NULL, 0);
      index = index + 2;
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
      index = index + 2;
    }
    else
    {
      index++;
    }
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

void run_pipe_cmd(std::vector<std::string> args)
{
  //there are some bugs here.
  //debug tomorrow(now is 4.7 22:30)
  int index = 0;
  std::vector<std::string> cmd;
  std::vector<std::string> cmd_pipe_right;
  while ((__SIZE_TYPE__)index < args.size())
  {
    cmd.push_back(args[index]);
    if (args[index] == "|")
    {
      cmd.pop_back();
      int fd[2];
      pipe(fd);
      pid_t pid1 = fork();
      if (pid1 == 0)
      {
        close(fd[0]);
        dup2(fd[1], 1);
        run_redi_cmd(cmd);
        cmd.clear();
        close(fd[1]);
      }
      for (__SIZE_TYPE__ i = index + 1; i < args.size(); i++)
      {
        cmd_pipe_right.push_back(args[i]);
      }
      pid_t pid2 = fork();
      if (pid2 == 0)
      {
        close(fd[1]);
        dup2(fd[0], 0);
        run_pipe_cmd(cmd_pipe_right);
        close(fd[0]);
      }
      close(fd[0]);
      close(fd[1]);
      waitpid(pid1, NULL, 0);
      waitpid(pid2, NULL, 0);
      //break; is very necessary there.
      //or the recursion is wrong.
      break;
    }
    index++;
  }
  index = 0;
  int count = 0;
  while ((__SIZE_TYPE__)index < args.size())
  {
    if (args[index] == "|")
    {
      count++;
    }
    index++;
  }
  if (count == 0)
    // there is no pipe
    run_redi_cmd(args);
  return;
}