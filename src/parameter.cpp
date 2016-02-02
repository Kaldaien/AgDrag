/**
 * This file is part of Agnostic Dragon.
 *
 * Agnostic Dragon is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Agnostic Dragon is distributed in the hope that it will be
 * useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Agnostic Dragon.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#include "parameter.h"

std::wstring
ad::ParameterInt::get_value_str (void)
{
  wchar_t str [32];
  _itow (value, str, 10);

  return std::wstring (str);
}

int
ad::ParameterInt::get_value (void)
{
  return value;
}

void
ad::ParameterInt::set_value (int val)
{
  value = val;
}


void
ad::ParameterInt::set_value_str (std::wstring str)
{
  value = _wtoi (str.c_str ());
}


std::wstring
ad::ParameterInt64::get_value_str (void)
{
  wchar_t str [32];
  _i64tow (value, str, 10);

  return std::wstring (str);
}

int64_t
ad::ParameterInt64::get_value (void)
{
  return value;
}

void
ad::ParameterInt64::set_value (int64_t val)
{
  value = val;
}


void
ad::ParameterInt64::set_value_str (std::wstring str)
{
  value = _wtol (str.c_str ());
}


std::wstring
ad::ParameterBool::get_value_str (void)
{
  if (value == true)
    return L"true";

  return L"false";
}

bool
ad::ParameterBool::get_value (void)
{
  return value;
}

void
ad::ParameterBool::set_value (bool val)
{
  value = val;
}


void
ad::ParameterBool::set_value_str (std::wstring str)
{
  if (str.length () == 1 &&
      str [0] == L'1')
    value = true;

  else if (str.length () == 4 &&
      towlower (str [0]) == L't' &&
      towlower (str [1]) == L'r' &&
      towlower (str [2]) == L'u' &&
      towlower (str [3]) == L'e')
    value = true;

  else
    value = false;
}


std::wstring
ad::ParameterFloat::get_value_str (void)
{
  wchar_t val_str [16];
  swprintf (val_str, L"%f", value);

  // Remove trailing 0's after the .
  int len = wcslen (val_str);
  for (int i = (len - 1); i > 1; i--) {
    if (val_str [i] == L'0' && val_str [i - 1] != L'.')
      len--;
    if (val_str [i] != L'0' && val_str [i] != L'\0')
      break;
  }

  val_str [len] = L'\0';

  return std::wstring (val_str);
}

float
ad::ParameterFloat::get_value (void)
{
  return value;
}

void
ad::ParameterFloat::set_value (float val)
{
  value = val;
}


void
ad::ParameterFloat::set_value_str (std::wstring str)
{
  value = (float)wcstod (str.c_str (), NULL);
}


std::wstring
ad::ParameterStringW::get_value_str (void)
{
  return value;
}

std::wstring
ad::ParameterStringW::get_value (void)
{
  return value;
}

void
ad::ParameterStringW::set_value (std::wstring val)
{
  value = val;
}


void
ad::ParameterStringW::set_value_str (std::wstring str)
{
  value = str;
}


template <>
ad::iParameter*
ad::ParameterFactory::create_parameter <int> (const wchar_t* name)
{
  iParameter* param = new ParameterInt ();
  params.push_back (param);

  return param;
}

template <>
ad::iParameter*
ad::ParameterFactory::create_parameter <int64_t> (const wchar_t* name)
{
  iParameter* param = new ParameterInt64 ();
  params.push_back (param);

  return param;
}

template <>
ad::iParameter*
ad::ParameterFactory::create_parameter <bool> (const wchar_t* name)
{
  iParameter* param = new ParameterBool ();
  params.push_back (param);

  return param;
}

template <>
ad::iParameter*
ad::ParameterFactory::create_parameter <float> (const wchar_t* name)
{
  iParameter* param = new ParameterFloat ();
  params.push_back (param);

  return param;
}

template <>
ad::iParameter*
ad::ParameterFactory::create_parameter <std::wstring> (const wchar_t* name)
{
  iParameter* param = new ParameterStringW ();
  params.push_back (param);

  return param;
}