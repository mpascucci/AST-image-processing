// Copyright 2019 Copyright 2019 The ASTapp Consortium
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    <http://www.apache.org/licenses/LICENSE-2.0>
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Marco Pascucci <marpas.paris@gmail.com>.

/* ASTapp custom exceptions */

#ifndef AST_EXCEPTIONS
#define AST_EXCEPTIONS

#include <exception>
#include <string>
#include <stdexcept>

using namespace std;

namespace astimp {
namespace Exception {
class generic : public std::runtime_error {
    /* @Brief Generic exception for ASTappCore.
     * usage: throw AST::Exception::generic("some custom message", __FILE__,
     * __LINE__);
     */

   public:
    explicit generic(const string &inputMessage = "generic exception",
                     const string &file = "", int line = -1)
        : runtime_error(inputMessage),
          msg(BuildFullMessage(inputMessage, file, line)){};

    string message() const { return msg; }

    const char* what() const throw() { return msg.c_str(); }

   private:
    string msg;

    string BuildFullMessage(const string &baseMessage, const string &file,
                                  int line) const {
        string out = "[improc Exception]";
        out += " <" + baseMessage + "> ";
        if (file != "") {
            out += " at " + file;
        };
        if (line > -1) {
            out += ":" + to_string(line);
        };
        return out;
    }
};
}  // namespace Exception
}  // namespace astimp

#endif  // AST_EXCEPTIONS
