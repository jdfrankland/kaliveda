//Created by KVClassFactory on Tue Jun 29 14:44:22 2010
//Author: bonnet

#ifndef __KVNAMEVALUELIST_H
#define __KVNAMEVALUELIST_H

#include "KVHashList.h"
#include "TNamed.h"
#include "TRegexp.h"
#include "KVNamedParameter.h"
class KVEnv;

class KVNameValueList : public TNamed {
protected:
   KVHashList fList;//list of KVNamedParameter objects
   Bool_t fIgnoreBool;//do not convert "yes", "false", "on", etc. in TEnv file to boolean

public:

   KVNameValueList();
   KVNameValueList(const Char_t* name, const Char_t* title = "");
   KVNameValueList(const KVNameValueList&);
   virtual ~KVNameValueList();
   KVNameValueList& operator=(const KVNameValueList&);

   KVHashList* GetList() const;

   virtual void Clear(Option_t* opt = "");
   virtual void ClearSelection(TRegexp&);
   virtual void Print(Option_t* opt = "") const;
   virtual void ls(Option_t* opt = "") const
   {
      Print(opt);
   }

   void SetOwner(Bool_t enable = kTRUE);
   Bool_t IsOwner() const;

   void Copy(TObject& nvl) const;
   Int_t Compare(const TObject* nvl) const;

   template<typename value_type>
   void SetValue(const Char_t* name, value_type value)
   {
      //associate a parameter (define by its name) and a value
      //if the parameter is not in the list, it is added
      //if it's in the list replace its value
      KVNamedParameter* par = FindParameter(name);
      par ? par->Set(name, value) : fList.Add(new KVNamedParameter(name, value));
   }
   void SetValue(const KVNamedParameter&);

   template <typename value_type>
   void SetValueAt(const Char_t* name, value_type value, Int_t idx)
   {
      //associate a parameter (define by its name) and a value
      //if the parameter is not in the list, it is inserted at
      //location idx in the list.
      //if it's in the list replace its value and move it at
      //location idx in the list.
      KVNamedParameter* par = FindParameter(name);
      if (par) {
         par->Set(value);
         if (fList.IndexOf(par) != idx) {
            fList.GetCollection()->Remove(par);
            fList.AddAt(par, idx);
         }
      } else fList.AddAt(new KVNamedParameter(name, value), idx);
   }

   template <typename value_type>
   void SetFirstValue(const Char_t* name, value_type value)
   {
      //associate a parameter (define by its name) and a value
      //if the parameter is not in the list, it is inserted at
      //the begining of the list.
      //if it's in the list replace its value and move it at
      //the begining of the list.
      KVNamedParameter* par = FindParameter(name);
      if (par) {
         par->Set(value);
         if (fList.First() != par) {
            fList.GetCollection()->Remove(par);
            fList.AddFirst(par);
         }
      } else fList.AddFirst(new KVNamedParameter(name, value));
   }

   template <typename value_type>
   void SetLastValue(const Char_t* name, value_type value)
   {
      //associate a parameter (define by its name) and a value
      //if the parameter is not in the list, it is inserted at
      //the end of the list.
      //if it's in the list replace its value and move it at
      //the end of the list.
      KVNamedParameter* par = FindParameter(name);
      if (par) {
         par->Set(value);
         if (fList.Last() != par) {
            fList.GetCollection()->Remove(par);
            fList.AddLast(par);
         }
      } else fList.AddLast(new KVNamedParameter(name, value));
   }

   template <typename value_type>
   value_type IncrementValue(const Char_t* name, value_type value)
   {
      //increment a parameter (define by its name) by a value
      //if the parameter is not in the list, it is added
      //if it's in the list increment its value
      //the new value of the parameter is returned
      KVNamedParameter* par = FindParameter(name);
      par ? par->Set(value += par->Get<value_type>()) : fList.Add(new KVNamedParameter(name, value));
      return value;
   }

   template <typename value_type>
   Bool_t IsValue(const Char_t* name, value_type value)
   {
      // Returns kTRUE if parameter with given name exists and is equal to given value
      KVNamedParameter* par = FindParameter(name);
      if (par) {
         KVNamedParameter tmp(name, value);
         return (*par) == tmp;
      }
      return kFALSE;
   }

   KVNamedParameter* FindParameter(const Char_t* name) const;
   KVNamedParameter* GetParameter(Int_t idx) const;
   void RemoveParameter(const Char_t* name);
   Bool_t HasParameter(const Char_t* name) const;
   template <typename value_type>
   Bool_t HasParameter(const Char_t* name) const
   {
      // Return kTRUE if list has parameter called 'name' and it is of given type

      KVNamedParameter* p = FindParameter(name);
      return (p && p->Is<value_type>());
   }
   Bool_t HasIntParameter(const Char_t* name) const
   {
      return HasParameter<int>(name);
   }
   Bool_t HasBoolParameter(const Char_t* name) const
   {
      return HasParameter<bool>(name);
   }
   Bool_t HasDoubleParameter(const Char_t* name) const
   {
      return HasParameter<double>(name);
   }
   Bool_t HasStringParameter(const Char_t* name) const
   {
      return HasParameter<TString>(name);
   }
   Int_t GetNameIndex(const Char_t* name);
   const Char_t* GetNameAt(Int_t idx) const;
   Int_t GetNpar() const;
   Int_t GetEntries() const
   {
      return GetNpar();
   }
   Bool_t IsEmpty() const
   {
      return GetNpar() == 0;
   }

   template <typename value_type>
   value_type GetValue(const Char_t* name) const
   {
      // return the value of named parameter
      // returns a default value (-1, false or "-1")
      // if no parameter with such a name is present

      KVNamedParameter* par = FindParameter(name);
      return (par ? par->Get<value_type>() : KVNamedParameter::DefaultValue<value_type>());
   }

   Int_t GetIntValue(const Char_t* name) const
   {
      return GetValue<int>(name);
   }
   Bool_t GetBoolValue(const Char_t* name) const
   {
      return GetValue<bool>(name);
   }
   Double_t GetDoubleValue(const Char_t* name) const
   {
      return GetValue<double>(name);
   }
   const Char_t* GetStringValue(const Char_t* name) const
   {
      return GetValue<cstring>(name);
   }
   TString GetTStringValue(const Char_t* name) const;

   template <typename value_type>
   value_type GetValue(Int_t idx) const
   {
      // return the value of parameter at position idx
      // returns a default value (-1, false or "-1") if idx is
      // greater than the number of stored parameters
      KVNamedParameter* par = GetParameter(idx);
      return (par ? par->Get<value_type>() : KVNamedParameter::DefaultValue<value_type>());
   }

   Int_t GetIntValue(Int_t idx) const
   {
      return GetValue<int>(idx);
   }
   Bool_t GetBoolValue(Int_t idx) const
   {
      return GetValue<bool>(idx);
   }
   Double_t GetDoubleValue(Int_t idx) const
   {
      return GetValue<double>(idx);
   }
   const Char_t* GetStringValue(Int_t idx) const
   {
      return GetValue<cstring>(idx);
   }
   TString GetTStringValue(Int_t idx) const;

   virtual void ReadEnvFile(const Char_t* filename);
   virtual KVEnv* ProduceEnvFile();
   virtual void WriteEnvFile(const Char_t* filename);

   void Sort(Bool_t order = kSortAscending)
   {
      fList.Sort(order);
   }

   KVNameValueList operator += (KVNameValueList& nvl);

   void WriteClass(const Char_t* classname, const Char_t* classdesc, const Char_t* base_class = "");

   void SetIgnoreBool(Bool_t ignore = kTRUE)
   {
      // When reading a list from a file using ReadEnvFile(), any string values matching
      // TRUE, FALSE, ON, OFF, YES, NO, OK, NOT are automatically converted to boolean
      // parameters. If you want to disable this and keep such strings as strings,
      // call this method first.
      fIgnoreBool = ignore;
   }
   void SetFromEnv(TEnv* tenv, const TString& prefix = "");
   void WriteToEnv(TEnv* tenv, const TString& prefix = "");

   ClassDef(KVNameValueList, 4) //A general-purpose list of parameters
};

#endif
