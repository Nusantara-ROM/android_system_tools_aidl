/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "options.h"

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>

#include <android-base/strings.h>

using android::base::Trim;
using std::endl;
using std::string;

namespace android {
namespace aidl {

string Options::GetUsage() const {
  std::ostringstream sstr;
  sstr << "usage:" << endl
       << myname_ << " --lang={java|cpp} [OPTION]... INPUT..." << endl
       << "   Generate Java or C++ files for AIDL file(s)." << endl
       << endl
       << myname_ << " --preprocess OUTPUT INPUT..." << endl
       << "   Create an AIDL file having declarations of AIDL file(s)." << endl
       << endl
       << myname_ << " --dumpapi OUTPUT INPUT..." << endl
       << "   Dump API signature of AIDL file(s)." << endl
       << endl;

  // Legacy option formats
  if (language_ == Options::Language::JAVA) {
    sstr << myname_ << " [OPTION]... INPUT [OUTPUT]" << endl
         << "   Generate a Java file for an AIDL file." << endl
         << endl;
  } else if (language_ == Options::Language::CPP) {
    sstr << myname_ << " [OPTION]... INPUT HEADER_DIR OUTPUT" << endl
         << "   Generate C++ headers and source for an AIDL file." << endl
         << endl;
  }

  sstr << "OPTION:" << endl
       << "  -I DIR, --include=DIR" << endl
       << "          Use DIR as a search path for import statements." << endl
       << "  -p FILE, --preprocessed=FILE" << endl
       << "          Include FILE which is created by --preprocess." << endl
       << "  -d FILE, --dep=FILE" << endl
       << "          Generate dependency file as FILE. Don't use this when" << endl
       << "          there are multiple input files. Use -a then." << endl
       << "  -o DIR, --out=DIR" << endl
       << "          Use DIR as the base output directory for generated files." << endl
       << "  -h DIR, --header_out=DIR" << endl
       << "          Generate C++ headers under DIR." << endl
       << "  -a" << endl
       << "          Generate dependency file next to the output file with the" << endl
       << "          name based on the input file." << endl
       << "  -b" << endl
       << "          Trigger fail when trying to compile a parcelable." << endl
       << "  --ninja" << endl
       << "          Generate dependency file in a format ninja understands." << endl
       << "  -t, --trace" << endl
       << "          Include tracing code for systrace. Note that if either" << endl
       << "          the client or service code is not auto-generated by this" << endl
       << "          tool, that part will not be traced." << endl
       << "  --transaction_names" << endl
       << "          Generate transaction names." << endl
       << "  --help" << endl
       << "          Show this help." << endl
       << endl
       << "INPUT:" << endl
       << "  An AIDL file." << endl
       << endl
       << "OUTPUT:" << endl
       << "  Path to the generated Java or C++ source file. This is ignored when" << endl
       << "  -o or --out is specified or the number of the input files are" << endl
       << "  more than one." << endl
       << "  For Java, if omitted, Java source file is generated at the same" << endl
       << "  place as the input AIDL file," << endl
       << endl
       << "HEADER_DIR:" << endl
       << "  Path to where C++ headers are generated." << endl;
  return sstr.str();
}

Options::Options(int argc, const char* const argv[], Options::Language default_lang)
    : myname_(argv[0]), language_(default_lang) {
  bool lang_option_found = false;
  optind = 1;
  while (true) {
    static struct option long_options[] = {
        {"lang", required_argument, 0, 'l'},
        {"preprocess", no_argument, 0, 's'},
        {"dumpapi", no_argument, 0, 'u'},
        {"include", required_argument, 0, 'I'},
        {"preprocessed", required_argument, 0, 'p'},
        {"dep", required_argument, 0, 'd'},
        {"out", required_argument, 0, 'o'},
        {"header_out", required_argument, 0, 'h'},
        {"ninja", no_argument, 0, 'n'},
        {"trace", no_argument, 0, 't'},
        {"transaction_names", no_argument, 0, 'c'},
        {"help", no_argument, 0, 'e'},
        {0, 0, 0, 0},
    };
    const int c =
        getopt_long(argc, const_cast<char* const*>(argv), "I:p:d:o:h:abt", long_options, nullptr);
    if (c == -1) {
      // no more options
      break;
    }

    switch (c) {
      case 'l':
        if (language_ == Options::Language::CPP) {
          // aidl-cpp can't set language. aidl-cpp exists only for backwards
          // compatibility.
          error_message_ << "aidl-cpp does not support --lang." << endl;
          return;
        } else {
          lang_option_found = true;
          string lang = Trim(string(optarg));
          if (lang == "java") {
            language_ = Options::Language::JAVA;
            task_ = Options::Task::COMPILE;
          } else if (lang == "cpp") {
            language_ = Options::Language::CPP;
            task_ = Options::Task::COMPILE;
          } else {
            error_message_ << "Unsupported language: '" << lang << "'" << endl;
            return;
          }
        }
        break;
      case 's':
        if (task_ != Options::Task::UNSPECIFIED) {
          task_ = Options::Task::PREPROCESS;
        }
        break;
      case 'u':
        if (task_ != Options::Task::UNSPECIFIED) {
          task_ = Options::Task::DUMPAPI;
        }
        break;
      case 'I':
        import_paths_.emplace_back(Trim(string(optarg)));
        break;
      case 'p':
        preprocessed_files_.emplace_back(Trim(string(optarg)));
        break;
      case 'd':
        dependency_file_ = Trim(string(optarg));
        break;
      case 'o':
        output_dir_ = Trim(string(optarg));
        break;
      case 'h':
        output_header_dir_ = Trim(string(optarg));
        break;
      case 'n':
        dependency_file_ninja_ = true;
        break;
      case 't':
        gen_traces_ = true;
        break;
      case 'a':
        auto_dep_file_ = true;
        break;
      case 'b':
        fail_on_parcelable_ = true;
        break;
      case 'c':
        gen_transaction_names_ = true;
        break;
      case 'e':
        std::cerr << GetUsage();
        exit(0);
      default:
        error_message_ << "Invalid argument: '" << argv[optind] << "'" << endl;
        return;
    }
  }  // while

  // Positional arguments
  if (!lang_option_found && task_ == Options::Task::COMPILE) {
    // the legacy arguments format
    if (argc - optind <= 0) {
      error_message_ << "No input file" << endl;
      return;
    }
    if (language_ == Options::Language::JAVA) {
      input_files_.emplace_back(argv[optind++]);
      if (argc - optind >= 1) {
        output_file_ = argv[optind++];
      } else {
        // when output is omitted, output is by default set to the input
        // file path with .aidl is replaced to .java.
        output_file_ = input_files_.front();
        output_file_.replace(output_file_.length() - strlen(".aidl"), strlen(".aidl"), ".java");
      }
    } else if (language_ == Options::Language::CPP) {
      input_files_.emplace_back(argv[optind++]);
      if (argc - optind < 2) {
        error_message_ << "No HEADER_DIR or OUTPUT." << endl;
        return;
      }
      output_header_dir_ = argv[optind++];
      output_file_ = argv[optind++];
    }
    if (argc - optind > 0) {
      error_message_ << "Too many arguments: ";
      for (int i = optind; i < argc; i++) {
        error_message_ << " " << argv[i];
      }
      error_message_ << endl;
    }
    return;
  } else {
    // the new arguments format
    if (task_ == Options::Task::COMPILE) {
      if (argc - optind < 1) {
        error_message_ << "No input file." << endl;
        return;
      }
    } else {
      if (argc - optind < 2) {
        error_message_ << "Insufficient arguments. At least 2 required, but "
                       << "got " << (argc - optind) << "." << endl;
        return;
      }
      output_file_ = argv[optind++];
    }
    while (optind < argc) {
      input_files_.emplace_back(argv[optind++]);
    }
  }

  // filter out invalid combinations
  for (const string& input : input_files_) {
    if (!android::base::EndsWith(input, ".aidl")) {
      error_message_ << "Expected .aidl file for input but got '" << input << "'" << endl;
      return;
    }
  }
  if (language_ == Options::Language::CPP && task_ == Options::Task::COMPILE) {
    if (output_dir_.empty()) {
      error_message_ << "Output directory is not set. Set with --out." << endl;
      return;
    }
    if (output_header_dir_.empty()) {
      error_message_ << "Header output directory is not set. Set with "
                     << "--header_out." << endl;
      return;
    }
  }
  if (language_ == Options::Language::JAVA && task_ == Options::Task::COMPILE) {
    if (output_dir_.empty()) {
      error_message_ << "Output directory is not set. Set with --out." << endl;
      return;
    }
    if (!output_header_dir_.empty()) {
      error_message_ << "Header output directory is set, which does not make "
                     << "sense for Java." << endl;
      return;
    }
  }
  if (task_ == Options::Task::COMPILE) {
    if (!output_file_.empty() && input_files_.size() > 1) {
      error_message_ << "Multiple AIDL files can't be compiled to a single "
                     << "output file '" << output_file_ << "'. "
                     << "Use --out=DIR instead for output files." << endl;
      return;
    }
    if (!dependency_file_.empty() && input_files_.size() > 1) {
      error_message_ << "-d or --dep doesn't work when compiling multiple AIDL "
                     << "files. Use '-a' to generate dependency file next to "
                     << "the output file with the name based on the input "
                     << "file." << endl;
      return;
    }
  }

}

}  // namespace android
}  // namespace aidl
