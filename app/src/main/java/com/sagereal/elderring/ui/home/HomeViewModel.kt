package com.sagereal.elderring.ui.home

import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel


object HomeViewModel : ViewModel() {

    val textMac = MutableLiveData<String>().apply {
        value = "MAC : "
    }
    val textMode = MutableLiveData<String>().apply {
        value = "MODE : "
    }
    val textState = MutableLiveData<String>().apply {
        value = "CONNECT : "
    }
    val textMessage = MutableLiveData<String>().apply {
        value = "MESSAGE : "
    }
    val buttonSendOffEnableState = MutableLiveData<Boolean>().apply { value = true }

    // signal: is my custom prefix value to mark ViewModel <-> Activity data variables
    val signalCallOff = MutableLiveData<String>()
    val signalCallConnect = MutableLiveData<String>()

    fun setConnectDeviceInfo(mac: String, state: String, mode: String) {
        textMac.postValue("MAC : $mac")
        textMode.postValue("MODE : $mode")
        textState.postValue("CONNECT : $state")
        if (state == "true")
            buttonSendOffEnableState.postValue(true)
        else
            buttonSendOffEnableState.postValue(false)
    }

    fun setReceiveMessage(text: String) {
        textMessage.postValue("MESSAGE : $text")
    }


}