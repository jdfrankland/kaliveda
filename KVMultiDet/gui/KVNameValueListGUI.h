//Created by KVClassFactory on Fri Feb 17 09:32:40 2017
//Author: John Frankland,,,

#ifndef __KVNAMEVALUELISTGUI_H
#define __KVNAMEVALUELISTGUI_H

#include "TGFrame.h"
#include "RQ_OBJECT.h"
#include "KVNameValueList.h"

#include <TGButton.h>
#include <TObjArray.h>

/**
\class KVNameValueListGUI
\brief GUI for setting KVNameValueList parameters
\ingroup GUI

This is a lightweight transient dialog box which can be used to set an arbitrary list of parameters.
The parameters are contained in a KVNameValueList. To use, do:

~~~~{.cpp}
KVNameValueList params{{"FixLimits?",false}, {"Min",0.0}, {"Max",100.0}};
bool cancel_pressed{false};

auto param_gui = new KVNameValueListGUI(nullptr, &params, &cancel_pressed);
param_gui->DisplayDialog();

if(!cancel_pressed)
{
   // values in param list now reflect user input
   params.Print();
}
~~~~

[Note that this example is for stand-alone use of the GUI. The first argument to the constructor
is usually the TGMainFrame of your GUI application]

If the user presses 'Cancel', the original values of the parameters are restored.

Boolean parameters can be used to 'enable' other parameters in the GUI, i.e. with the
previous example, if you do:

~~~~{.cpp}
param_gui->EnableDependingOnBool("Min","FixLimits?");
param_gui->EnableDependingOnBool("Max","FixLimits?");
param_gui->DisplayDialog();
~~~~

then the "Min" and "Max" input widgets in the GUI will only be enabled when the "FixLimits?" parameter
is `true`, and will be en/disabled when the check box for this parameter is checked/unchecked.
*/

class KVNameValueListGUI {
   RQ_OBJECT("KVNameValueListGUI")
private:
   KVNameValueList* theList;
   KVNameValueList fOriginal;

   TGTransientFrame* fMain;
   TGTextButton* fOKBut;        //OK button
   TGTextButton* fCancelBut;    //Cancel button
   Bool_t* fOK;                 //set to kTRUE if OK button is pressed
   UInt_t max_width;
   UInt_t fWidth, fHeight;
   TObjArray fData;

   Bool_t* fCancel;
   Bool_t fWaitForMain;

protected:
   virtual TObject* AddAString(Int_t i, TGHorizontalFrame* hf);
   virtual TObject* AddABool(Int_t i, TGHorizontalFrame* hf);
   virtual TObject* AddADouble(Int_t i, TGHorizontalFrame* hf);
   virtual TObject* AddAInt(Int_t i, TGHorizontalFrame* hf);

   TObject* GetDataWidget(int i) const
   {
      return fData[i];
   }
   KVNameValueList* GetList() const
   {
      return theList;
   }
   TGTransientFrame* GetMain() const
   {
      return fMain;
   }
   TObject* GetDataWidget(const TString& parname) const
   {
      // Return pointer to widget corresponding to named parameter in list

      auto index = theList->GetNameIndex(parname);
      if (index < 0) return nullptr;
      return GetDataWidget(index);
   }

public:
   KVNameValueListGUI(const TGWindow* main, KVNameValueList* params, Bool_t* cancel_pressed, Bool_t wait_for_main = kTRUE);

   virtual ~KVNameValueListGUI();

   void ReadData();
   void RestoreData();
   void DoClose();
   void CloseWindow();

   bool EnableDependingOnBool(const TString& value_to_enable, const TString& bool_parameter);

   ClassDef(KVNameValueListGUI, 1) //GUI for setting KVNameValueList parameters
   void DisplayDialog();
};

#endif
