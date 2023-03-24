package com.sagereal.elderring.ui.home

import android.util.Log
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext


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
    var textMac = _textMac
    var textState = _textState
    var textState110 = _textState110
    var textState131 = _textState131

    fun shit(mac: String, state: Int) {
        Log.wtf("[CCMETA]mac", mac)
        Log.wtf("[CCMETA]state", "$state")
        Log.wtf("[CCMETA]", "call shit done")
        textMac.postValue(textMac.value + mac)
        textState.postValue(textState.value + state)
    }
}