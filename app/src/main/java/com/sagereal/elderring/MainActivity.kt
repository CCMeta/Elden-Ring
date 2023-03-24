package com.sagereal.elderring

import android.Manifest
import android.bluetooth.*
import android.bluetooth.BluetoothDevice.TRANSPORT_LE
import android.bluetooth.le.BluetoothLeScanner
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.lifecycle.lifecycleScope
import androidx.navigation.findNavController
import androidx.navigation.ui.AppBarConfiguration
import androidx.navigation.ui.setupActionBarWithNavController
import androidx.navigation.ui.setupWithNavController
import com.google.android.material.bottomnavigation.BottomNavigationView
import com.sagereal.elderring.databinding.ActivityMainBinding
import com.sagereal.elderring.ui.home.HomeViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.util.*
import kotlin.concurrent.thread


class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var bluetoothManager: BluetoothManager
    private lateinit var bluetoothAdapter: BluetoothAdapter
    private lateinit var bluetoothLeScanner: BluetoothLeScanner
    private lateinit var context: Context
    private var bluetoothGatt: BluetoothGatt? = null
    val UUID_Service = UUID.randomUUID()
    val UUID_Characteristic = UUID.randomUUID()
    val UUID_Descriptor = UUID.randomUUID()

    private val TARGET_MAC: String = "F4:4E:FC:00:00:01" // CB01
//    private val TARGET_MAC: String = "AC:90:85:0B:77:C2" // AIRPODS
//    private val TARGET_MAC: String = "E4:19:C1:C3:29:DD" // HONOR

    @RequiresApi(Build.VERSION_CODES.S)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        context = this
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val navView: BottomNavigationView = binding.navView

        val navController = findNavController(R.id.nav_host_fragment_activity_main)
        // Passing each menu ID as a set of Ids because each
        // menu should be considered as top level destinations.
        val appBarConfiguration = AppBarConfiguration(
            setOf(
                R.id.navigation_home, R.id.navigation_dashboard, R.id.navigation_notifications
            )
        )
        setupActionBarWithNavController(navController, appBarConfiguration)
        navView.setupWithNavController(navController)

        val fuck = arrayOf(
            Manifest.permission.BLUETOOTH_SCAN,
            Manifest.permission.BLUETOOTH,
            Manifest.permission.BLUETOOTH_ADMIN,
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.BLUETOOTH_CONNECT
        )
        requestPermissions(fuck, 100)
        if (ActivityCompat.checkSelfPermission(
                this,
                Manifest.permission.BLUETOOTH_ADMIN
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            cc("checkSelfPermission BLUETOOTH_SCAN failed 86")
            return
        }

        bluetoothManager = this.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothAdapter = bluetoothManager.adapter

        val registerReceiver =
            registerReceiver(receiver, IntentFilter(BluetoothDevice.ACTION_FOUND))

        if (bluetoothAdapter.startDiscovery()) {
//            while (bluetoothAdapter.isDiscovering) {
//                cc(" Thread.sleep(1000)")
//                Thread.sleep(1000)
//            }
        }

//        startBLE()
//        startClassicBT(bluetoothManager.adapter.getRemoteDevice(TARGET_MAC))
    }

    // Create a BroadcastReceiver for ACTION_FOUND.
    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            when (intent.action) {
                BluetoothDevice.ACTION_FOUND -> {
//                    cc("asd")
                    if (ActivityCompat.checkSelfPermission(
                            context,
                            Manifest.permission.BLUETOOTH_ADMIN
                        ) != PackageManager.PERMISSION_GRANTED
                    ) {
                        cc("checkSelfPermission BLUETOOTH_CONNECT failed 116")
                        return
                    }
                    val device: BluetoothDevice =
                        intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE) ?: return
                    cc(device.name)
                    cc(device.address)

                    if (device.address == "F4:4E:FC:00:00:01") {
                        startClassicBT(device)
                    }
                }
            }
        }
    }

    @RequiresApi(Build.VERSION_CODES.S)
    private fun startBLE() {
        bluetoothLeScanner = bluetoothManager.adapter.bluetoothLeScanner
        if (ActivityCompat.checkSelfPermission(
                this, Manifest.permission.BLUETOOTH_ADMIN
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            val fuck = arrayOf(
                Manifest.permission.BLUETOOTH_SCAN,
                Manifest.permission.BLUETOOTH,
                Manifest.permission.BLUETOOTH_ADMIN,
                Manifest.permission.ACCESS_FINE_LOCATION,
                Manifest.permission.ACCESS_COARSE_LOCATION,
                Manifest.permission.BLUETOOTH_CONNECT
            )
            requestPermissions(fuck, 100)
            Log.wtf("[CCMETA]", "checkSelfPermission BLUETOOTH_SCAN failed 118")
            return
        }
        Log.wtf("[CCMETA]", "bluetoothLeScanner.startScan(mScanCallback)")
        bluetoothLeScanner.startScan(mScanCallback) // 開始搜尋
//        Log.wtf("[CCMETA]", "bluetoothLeScanner.stopScan(mScanCallback)")
//        bluetoothLeScanner.stopScan(mScanCallback) // 停止搜尋
    }

    private val mScanCallback = object : ScanCallback() {
        override fun onScanFailed(errorCode: Int) {
            super.onScanFailed(errorCode)
            Log.wtf("[CCMETA]", "onScanFailed")
        }

        @RequiresApi(Build.VERSION_CODES.S)
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            super.onScanResult(callbackType, result)
            if (ActivityCompat.checkSelfPermission(
                    context, Manifest.permission.BLUETOOTH_ADMIN
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                val fuck = arrayOf(
                    Manifest.permission.BLUETOOTH,
                    Manifest.permission.BLUETOOTH_ADMIN,
                    Manifest.permission.ACCESS_FINE_LOCATION,
                    Manifest.permission.ACCESS_COARSE_LOCATION,
                    Manifest.permission.BLUETOOTH_CONNECT,
                )
                requestPermissions(fuck, 100)
                Log.wtf("[CCMETA]", "checkSelfPermission BLUETOOTH_CONNECT failed 149")
                return
            }
            val device = result?.device
//            Log.wtf("[CCMETA]", "$deviceName")
            if (device?.name != null && device.name.isNotEmpty()) {
//                mLeDeviceListAdapter!!.addDevice(result.device!!)
//                mLeDeviceListAdapter!!.notifyDataSetChanged()

//                Log.wtf("[CCMETA] ScanResult ", "$result")
//                Log.wtf("[CCMETA] device.address ", device.address)
//                Log.wtf("[CCMETA] device.name", device.name)
                if (TARGET_MAC != result.device.address) return
                if (!result.isConnectable) {
                    Log.wtf("[CCMETA] isConnectable", result.isConnectable.toString())
                    return
                }


                // CLASSIC CONNECT MODE DIFF WITH GATT
//                startClassicBT(device)
//                return


                // GATT CONNECT MODE DIFF WITH CLASSIC
                if (bluetoothGatt != null) {
                    bluetoothGatt!!.disconnect()
                    bluetoothGatt!!.close()
                    bluetoothGatt = null
                }
                bluetoothGatt =
                    device.connectGatt(context, true, object : BluetoothGattCallback() {

                        override fun onConnectionStateChange(
                            gatt: BluetoothGatt, status: Int, newState: Int
                        ) {
                            super.onConnectionStateChange(gatt, status, newState)
                            Log.wtf(
                                "[CCMETA] status", status.toString()
                            )
                            Log.wtf(
                                "[CCMETA] newState", newState.toString()
                            )
                            lifecycleScope.launch {
                                withContext(Dispatchers.Main) {
                                    HomeViewModel.shit(gatt.device.address, newState)
                                }
                            }
                        }

                        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
                            Log.wtf(
                                "[CCMETA] onServicesDiscovered", "onServicesDiscovered "
                            )
                            super.onServicesDiscovered(gatt, status)
                            val bluetoothGattService = gatt.getService(UUID_Service)
                            val bluetoothGattCharacteristic =
                                bluetoothGattService.getCharacteristic(UUID_Characteristic)
                            val bluetoothGattDescriptor =
                                bluetoothGattCharacteristic.getDescriptor(UUID_Descriptor)
                            if (ActivityCompat.checkSelfPermission(
                                    context, Manifest.permission.BLUETOOTH_CONNECT
                                ) != PackageManager.PERMISSION_GRANTED
                            ) {
                                Log.wtf(
                                    "[CCMETA] checkSelfPermission",
                                    "checkSelfPermission failed L148"
                                )
                                return
                            }
                            gatt.setCharacteristicNotification(bluetoothGattCharacteristic, true)
                            bluetoothGattDescriptor.value =
                                BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                            gatt.writeDescriptor(bluetoothGattDescriptor)
                        }

                        override fun onDescriptorWrite(
                            gatt: BluetoothGatt?, descriptor: BluetoothGattDescriptor?, status: Int
                        ) {
                            Log.wtf(
                                "[CCMETA] onDescriptorWrite", "onDescriptorWrite "
                            )
                            super.onDescriptorWrite(gatt, descriptor, status)
                            val bluetoothGattService = gatt?.getService(UUID_Service)
                            val bluetoothGattCharacteristic =
                                bluetoothGattService?.getCharacteristic(UUID_Characteristic)
                            bluetoothGattCharacteristic?.value = byteArrayOf(
                                0xA1.toByte(),
                                0x2E.toByte(),
                                0x38.toByte(),
                                0xD4.toByte(),
                                0x89.toByte(),
                                0xC3.toByte()
                            )
                            if (ActivityCompat.checkSelfPermission(
                                    context, Manifest.permission.BLUETOOTH_CONNECT
                                ) != PackageManager.PERMISSION_GRANTED
                            ) {
                                Log.wtf(
                                    "[CCMETA] checkSelfPermission",
                                    "checkSelfPermission failed L185"
                                )
                                return
                            }
                            gatt?.writeCharacteristic(bluetoothGattCharacteristic)
                        }
                    }, TRANSPORT_LE) // THIS IS MUST NOT AUTO, FIX THE STATUS=133 ON 230324
                Log.wtf("[CCMETA]", "finish a new connection.")
            }
        }
    }

    companion object {
//        var myUUID: UUID = UUID.fromString("00007400-0000-1000-8000-00805f9b34fb")

        var myUUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
        var mBluetoothSocket: BluetoothSocket? = null
        var isBlueConnected: Boolean = false
        const val MESSAGE_RECEIVE_TAG = 111
        lateinit var blueAddress: String
        lateinit var blueName: String
        private val BUNDLE_RECEIVE_DATA = "ReceiveData"
        private val TAG = "BlueDeviceActivity"

        //设置发送和接收的字符编码格式
        private val ENCODING_FORMAT = "GBK"
    }

    private fun startClassicBT(device: BluetoothDevice) {
        thread {
            if (ActivityCompat.checkSelfPermission(
                    this,
                    Manifest.permission.BLUETOOTH_ADMIN
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                Log.wtf("[CCMETA]", "checkSelfPermission BLUETOOTH_SCAN 301")
                return@thread
            }
            bluetoothAdapter.cancelDiscovery()

            if (mBluetoothSocket != null && isBlueConnected)
                finish()

            Log.wtf("[CCMETA]", "startClassicBT thread")

            val mBluetoothSocket: BluetoothSocket by lazy(LazyThreadSafetyMode.NONE) {
                device.createRfcommSocketToServiceRecord(myUUID)
            }
            Log.wtf("[CCMETA] isConnected", mBluetoothSocket.isConnected.toString())
            if (!mBluetoothSocket.isConnected) {
                mBluetoothSocket.use { socket -> socket.connect() }
            }
            val mmBuffer = ByteArray(1024)
            while (true) {
                if (mBluetoothSocket.inputStream.read(mmBuffer) > 0)
                    break
            }


            Log.wtf("[CCMETA]mBluetoothSocket", mBluetoothSocket.toString())

            if (mBluetoothSocket.isConnected) {
                Log.wtf("[CCMETA]", "mBluetoothSocket?.isConnected == true")

            }
            while (true) {
                Log.wtf("[CCMETA] isConnected", mBluetoothSocket?.isConnected.toString())
            }


        }
    }

    private fun cc(text: String?) {
        Log.wtf("[CCMETA]", text)

    }

}















