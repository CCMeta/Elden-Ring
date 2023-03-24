package com.sagereal.elderring

import android.app.Service
import android.bluetooth.*
import android.content.Context
import android.content.Intent
import android.os.BatteryManager
import android.os.IBinder
import android.os.VibratorManager
import android.util.Log

class DoroService : Service() {

    override fun onBind(intent: Intent): IBinder {
        TODO("Return the communication channel to the service.")
    }

}