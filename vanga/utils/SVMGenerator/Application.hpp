/* 
 * This file is part of the Vanga distribution (https://github.com/yoori/vanga).
 * Vanga is library that implement multinode decision tree constructing algorithm
 * for regression prediction
 *
 * Copyright (c) 2014 Yuri Kuznecov <yuri.kuznecov@gmail.com>.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SVMGENERATOR_APPLICATION_HPP_
#define SVMGENERATOR_APPLICATION_HPP_

#include <map>
#include <set>
#include <string>

#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/Singleton.hpp>
#include <Gears/Basic/Time.hpp>

class Application_
{
public:
  DECLARE_GEARS_EXCEPTION(Exception, Gears::DescriptiveException);

  Application_() throw();

  virtual ~Application_() throw();

  void main(int& argc, char** argv) throw(Gears::Exception);

protected:
  void
  generate_svm_(
    std::ostream& out,
    std::istream& in,
    const char* config_file,
    const char* feature_columns_str,
    const char* dictionary_file_path);

  void
  load_dictionary_(
    std::map<std::string, std::string>& dict,
    const char* file);
};

typedef Gears::Singleton<Application_> Application;

#endif /*SVMGENERATOR_APPLICATION_HPP_*/
