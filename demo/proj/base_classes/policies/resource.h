#pragma once
#include "utility.h"

namespace code_policy
{

template<class m_policy>
struct ResourceControl
{
  static string LoadText(string const&name)                    { return m_policy::LoadText(name);   }
  static vector<char> Load(string const&name)                  { return m_policy::Load(name);       }
  static bool Save(string const&name, vector<char> const&data) { return m_policy::Save(name, data); }
};


template<class m_policy>
struct ImageDecoderControl
{
  template<class T> static Image<T> Decode(vector<char> const&data, uint channels, bool flip=true) { return m_policy::template Decode<T>(data, channels, flip); }
  static vector<char> Encode(uImage img, bool flip=true) { return m_policy::Encode(move(img), flip); }
  static vector<char> Encode(fImage img, bool flip=true) { return m_policy::Encode(move(img), flip); }
};


template<class m_policy>
struct CompressionControl
{
  static Archive Compress(vector<char> const&data) { return m_policy::Compress(data); }
  static vector<char> Extract(Archive const&a)     { return m_policy::Extract(a);     }
};


struct OnDemandResourcePolicy
{
  static string LoadText(string const&);
  static vector<char> Load(string const&);
  static bool Save(string const&, vector<char> const&);
};


struct StbDecodePolicy
{
  template<class T> static Image<T> Decode(vector<char> const&, uint, bool);
  static vector<char> Encode(uImage, bool);
  static vector<char> Encode(fImage, bool);
};


struct LodepngDecodePolicy
{
  template<class T> static Image<T> Decode(vector<char> const&, uint, bool);
  static vector<char> Encode(uImage, bool);
  static vector<char> Encode(fImage, bool);
};


struct LZ4CompressionPolicy
{
  static Archive Compress(vector<char> const&);
  static vector<char> Extract(Archive const&);
};


typedef ResourceControl<OnDemandResourcePolicy> Resource;

typedef ImageDecoderControl<StbDecodePolicy> ImageCodec;

typedef CompressionControl<LZ4CompressionPolicy> Compression;

}
