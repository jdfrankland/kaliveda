//Created by KVClassFactory on Wed Nov 30 13:54:07 2011
//Author: John Frankland,,,

#ifndef __KVNAMEDPARAMETER_H
#define __KVNAMEDPARAMETER_H

#include "TNamed.h"
#include "TEnv.h"

typedef const char* cstring;

class KVNamedParameter : public TNamed {

   enum {
      kIsString = BIT(14),
      kIsDouble = BIT(15),
      kIsInt = BIT(16),
      kIsBool = BIT(17)
   };
   template <typename T>
   struct typecode {};

   enum {
      kBitMask = 0x0003c000
   };
   void ResetBits()
   {
      ResetBit(kBitMask);
   }

   Int_t GetType() const
   {
      // compare this value with the values kIsString, kIsDouble, etc.
      return TestBits(kBitMask);
   }

   void SetType(UInt_t f)
   {
      ResetBits();
      SetBit(f);
   }

protected:
   Double_t fNumber;//used to store numerical (integer or floating-point) values

public:
   KVNamedParameter();
   KVNamedParameter(const char*);
   KVNamedParameter(const char*, const char*);
   KVNamedParameter(const char*, Double_t);
   KVNamedParameter(const char*, Int_t);
   KVNamedParameter(const char*, Bool_t);
   KVNamedParameter(const char*, const KVNamedParameter&);
   virtual ~KVNamedParameter();

   void Set(const char*, const char*);
   void Set(const char*, Double_t);
   void Set(const char*, Int_t);
   void Set(const char*, Bool_t);
   void Set(const char*, const KVNamedParameter&);
   void Set(const char*);
   void Set(Double_t);
   void Set(Int_t);
   void Set(Bool_t);
   //void Set(const KVNamedParameter&);
   void Set(TEnv*, const TString& p = "");

   const Char_t* GetString() const;
   TString GetTString() const;
   Double_t GetDouble() const;
   Int_t GetInt() const;
   Bool_t GetBool() const;

   template <typename T>
   T Get() const
   {
      // compile-time error if one of allowed specializations not used
      T a;
      a.very_unlikely_method_name();
   }
   template <typename T>
   static T DefaultValue();
   virtual void Clear(Option_t* = "");

   template <typename T>
   Bool_t Is() const;
   Bool_t IsString() const
   {
      return GetType() == kIsString;
   }
   Bool_t IsDouble() const
   {
      return GetType() == kIsDouble;
   }
   Bool_t IsInt() const
   {
      return GetType() == kIsInt;
   }
   Bool_t IsBool() const
   {
      return GetType() == kIsBool;
   }
   Bool_t IsNumber() const
   {
      return (IsDouble() || IsInt());
   }

   Bool_t   IsEqual(const TObject* obj) const;
   Bool_t operator== (const KVNamedParameter&) const;
   Bool_t HasSameValueAs(const KVNamedParameter&) const;
   virtual void Print(Option_t* opt = "") const;
   virtual void ls(Option_t* opt = "") const;

   Int_t Compare(const TObject* obj) const;
   void WriteToEnv(TEnv*, const TString& p = "");

   const Char_t* GetSQLType() const;

   ClassDef(KVNamedParameter, 1) //A generic parameter with a name and a value
};

template<> inline int KVNamedParameter::Get<int>() const
{
   return GetInt();
}
template<> inline double KVNamedParameter::Get<double>() const
{
   return GetDouble();
}
template<> inline bool KVNamedParameter::Get<bool>() const
{
   return GetBool();
}
template<> inline TString KVNamedParameter::Get<TString>() const
{
   return GetTString();
}
template<> inline cstring KVNamedParameter::Get<cstring>() const
{
   return GetString();
}
template<> inline std::string KVNamedParameter::Get<std::string>() const
{
   return std::string(GetString());
}
template<> inline int KVNamedParameter::DefaultValue<int>()
{
   return -1;
}
template<> inline double KVNamedParameter::DefaultValue<double>()
{
   return -1.0;
}
template<> inline bool KVNamedParameter::DefaultValue<bool>()
{
   return false;
}
template<> inline TString KVNamedParameter::DefaultValue<TString>()
{
   return "-1";
}
template<> inline cstring KVNamedParameter::DefaultValue<cstring>()
{
   return "-1";
}
template<> inline std::string KVNamedParameter::DefaultValue<std::string>()
{
   return "-1";
}
template<> struct KVNamedParameter::typecode<int> {
   static const int VALUE = KVNamedParameter::kIsInt;
};
template<> struct KVNamedParameter::typecode<double> {
   static const int VALUE = KVNamedParameter::kIsDouble;
};
template<> struct KVNamedParameter::typecode<bool> {
   static const int VALUE = KVNamedParameter::kIsBool;
};
template<> struct KVNamedParameter::typecode<std::string> {
   static const int VALUE = KVNamedParameter::kIsString;
};
template<> struct KVNamedParameter::typecode<cstring> {
   static const int VALUE = KVNamedParameter::kIsString;
};
template<> struct KVNamedParameter::typecode<TString> {
   static const int VALUE = KVNamedParameter::kIsString;
};
template <typename T>
inline Bool_t KVNamedParameter::Is() const
{
   return GetType() == typecode<T>::VALUE;
}
#endif

