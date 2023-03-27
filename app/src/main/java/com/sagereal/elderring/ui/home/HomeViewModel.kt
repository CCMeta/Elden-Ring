package com.sagereal.elderring.ui.home

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel


object HomeViewModel : ViewModel() {

    private val _textMac = MutableLiveData<String>().apply {
        value = "MAC : "
    }
    private val _textState = MutableLiveData<String>().apply {
        value = "STATE : "
    }
    private val _textState110 = MutableLiveData<String>().apply {
        value = "STATE_110 : "
    }
    private val _textState131 = MutableLiveData<String>().apply {
        value = "STATE_131 : "
    }
    private val mutableSelectedItem = MutableLiveData<String>()
    val selectedItem: LiveData<String> get() = mutableSelectedItem
    var textMac = _textMac
    var textState = _textState
    var textState110 = _textState110
    var textState131 = _textState131

    fun selectItem(text: String) {
        mutableSelectedItem.value = text
    }

    fun setConnectDeviceInfo(mac: String, state: String, mode: String = "BLE") {
//        Log.wtf("[CCMETA]mac", mac)
//        Log.wtf("[CCMETA]state", state + "MODE=" + mode)
//        Log.wtf("[CCMETA]", "call shit done")
        textMac.postValue(textMac.value + mac)
        textState.postValue(textState.value + state + " | MODE=" + mode)
    }

    fun setConnectState(text: String) {
        textState110.postValue(textState110.value + text)
    }


}