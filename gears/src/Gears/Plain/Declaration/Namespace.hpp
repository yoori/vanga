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

#ifndef _NAMESPACE_HPP_
#define _NAMESPACE_HPP_

#include <list>
#include <map>
#include <Gears/Basic/Exception.hpp>
#include <Gears/Basic/AtomicRefCountable.hpp>
#include "BaseType.hpp"

namespace Declaration
{
  class NamePath: public std::list<std::string>
  {
  public:
    DECLARE_GEARS_EXCEPTION(InvalidName, Gears::DescriptiveException);
    
    NamePath(const char* abs_path, bool name_is_local = false)
      /*throw(InvalidName)*/;

    std::string str() const noexcept;
  };

  class Namespace;

  typedef Gears::IntrusivePtr<Namespace> Namespace_var;

  class Namespace: public virtual Gears::AtomicRefCountable
  {
  public:
    DECLARE_GEARS_EXCEPTION(AlreadyDefined, Gears::DescriptiveException);

    typedef std::map<std::string, BaseType_var>
      BaseTypeMap;

    typedef std::map<std::string,
      Gears::IntrusivePtr<Namespace> >
      NamespaceMap;
    
  public:
    Namespace(
      const char* name_val = 0,
      Namespace* owner_val = 0)
      noexcept;

    const char* name() const noexcept;

    NamePath abs_name() const noexcept;

    Namespace_var owner() const noexcept;

    const BaseTypeMap& types() const noexcept;

    const NamespaceMap& namespaces() const noexcept;

    BaseType_var find_type(const NamePath& name) const noexcept;

    BaseType_var find_local_type(const char* name) const noexcept;

    Gears::IntrusivePtr<Namespace>
    add_namespace(const char* name) noexcept;

    void add_type(BaseType*) /*throw(AlreadyDefined)*/;

  private:
    virtual ~Namespace() noexcept {}
    
    BaseType_var local_find_type_(const NamePath& name) const noexcept;
    
  private:
    std::string name_;
    Namespace* owner_;
    BaseTypeMap types_;
    NamespaceMap namespaces_;
  };

  typedef Gears::IntrusivePtr<Namespace>
    Namespace_var;
}

#endif /*_NAMESPACE_HPP_*/
