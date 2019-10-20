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

#ifndef GEARS_BASIC_MACRO_HPP
#define GEARS_BASIC_MACRO_HPP

#ifdef __cplusplus

#  define OPEN_NAMESPACE(NAME) namespace NAME {
#  define CLOSE_NAMESPACE }
#  define INTRUSIVE_PTR(CLASSNAME) RefCounting::IntrusivePtr<CLASSNAME>
#  define DEFINE_INTRUSIVE_PTR(CLASSNAME) typedef RefCounting::IntrusivePtr<CLASSNAME> CLASSNAME##_var
#  define NAMESPACE(NAME) NAME

#else

#  define OPEN_NAMESPACE(NAME)
#  define CLOSE_NAMESPACE
#  define INTRUSIVE_PTR(CLASSNAME) void*
#  define DEFINE_INTRUSIVE_PTR(CLASSNAME) typedef void* CLASSNAME##_var
#  define NAMESPACE(NAME)

#endif

#endif // GEARS_BASIC_MACRO_HPP
