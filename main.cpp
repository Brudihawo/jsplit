#include "nlohmann_json.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>

enum Program {
  ERROR = -1,
  PRINT_HELP,
  EXECUTE,
};

static std::vector<std::string> split(const std::string &str, char chr) {
  std::vector<std::string> parts;
  std::string cur_str = str;
  while (true) {
    auto val = std::find(cur_str.begin(), cur_str.end(), chr);
    if (val == cur_str.end()) {
      parts.push_back(cur_str);
      break;
    } else {
      std::string cur_part;
      std::copy(cur_str.begin(), val, std::back_inserter(cur_part));
      parts.push_back(cur_str);
    }
  }

  return parts;
}

struct Args {
  std::string filename;
  std::string program_name;
  std::vector<std::string> invalid_args;
  std::optional<std::string> out_folder;
  Program command = ERROR;
  bool opts_done;

  std::vector<std::string> unique_path;

  void parse(int argc, char **argv) {
    this->program_name = argv[0];

    int i = 1;
    for (; i < argc; ++i) {
      std::string cur_arg = argv[i];
      // parse optional arguments
      if (!this->parse_option(cur_arg)) {
        break;
      }
    }

    if (i > argc) {
      this->command = ERROR;
      return;
    }

    this->filename = argv[i++];

    for (; i < argc; ++i) {
      std::string cur_arg = argv[i];
      this->unique_path.push_back(cur_arg);
    }

    this->command = EXECUTE;
  }

  bool parse_option(const std::string &arg) {
    auto res = split(arg, '=');
    const std::string key = res[0];
    if (key[0] == '-') {
      if (res.size() == 1) {
        if (key == "--help" || key == "-h") {
          this->command = PRINT_HELP;
          return true;
        }
        this->command = ERROR;
        this->invalid_args.push_back(arg);
        return true;
      }

      if (key == "--out-folder" || key == "-o") {
        this->out_folder = res[1];
        return true;
      }

      if (res.size() > 2) {
        this->invalid_args.push_back(arg);
        return true;
      }
    }

    return false;
  }

  void print_help() const {
    std::cerr << "Usage: " << program_name
              << " [options] <file.json> json path identifiers\n";
    std::cerr << "    Splits ndjson file multiple files according\n"
              << "    to values at json path identifiers\n\n";

    if (this->invalid_args.size() > 0) {
      std::cerr << "Invalid Arguments were provided: \n";
      for (const auto &arg : this->invalid_args) {
        std::cerr << "    " << arg << "\n";
      }
    }

    std::cerr
        << "Options:\n"
        << "    -h / --help                             print this help\n"
        << "    -o=<folder> / --out-folder=<folder>     print this help\n";
  }
};

using tp = std::chrono::time_point<std::chrono::high_resolution_clock>;
void print_progress(const tp &cur, const tp &start, size_t filesize, size_t pos,
                    const std::string &target_name) {

  const auto elapsed = cur - start;
  const auto avg = elapsed / (double)pos;
  const auto total_expected = avg * filesize;
  const auto left = total_expected - elapsed;

  std::cout << "\e[A\r\e[0K"
            << "Processing... (" << std::setw(6) << std::setprecision(2)
            << (double)pos / (double)filesize * 100.0 << "% ) "
            << "[ETA: " << std::setw(6)
            << std::chrono::duration_cast<std::chrono::seconds>(left).count()
            << "s] " << target_name << std::endl;
}

int main(int argc, char **argv) {
  Args args;
  args.parse(argc, argv);
  switch (args.command) {
  case ERROR:
    args.print_help();
    return EXIT_FAILURE;
  case PRINT_HELP:
    args.print_help();
    return EXIT_SUCCESS;
  case EXECUTE:
    break;
  }

  if (!std::filesystem::exists(args.filename)) {
    std::cerr << "Could not open file: " << args.filename
              << ". File does not exist.\n";
    return EXIT_FAILURE;
  }
  if (!std::filesystem::is_regular_file(args.filename)) {
    std::cerr << "Could not open file: " << args.filename << ". Not a file.\n";
    return EXIT_FAILURE;
  }

  std::cout << "Processing file " << args.filename << "\n";
  std::ifstream file(args.filename);
  std::map<std::string, std::ofstream> outfiles;

  std::string line;
  const size_t filesize = std::filesystem::file_size(args.filename);
  size_t pos = 0;

  auto start = std::chrono::high_resolution_clock::now();
  while (std::getline(file, line)) {
    auto j = nlohmann::json::parse(line);
    std::string target_name = j.at("inparams").at("target");
    if (outfiles.find(target_name) == outfiles.end()) {
      const std::string outfile_name = target_name + ".json";
      outfiles[target_name] = std::ofstream(
          std::filesystem::path(args.out_folder.value_or(".")) / outfile_name);
    }
    outfiles.at(target_name) << j << "\n";

    pos += line.size();
    const auto cur = std::chrono::high_resolution_clock::now();
    print_progress(cur, start, filesize, pos, target_name);
  }
}
