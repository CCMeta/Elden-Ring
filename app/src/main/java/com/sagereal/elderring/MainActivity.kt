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
import android.os.Handler
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


@Suppress("ConvertToStringTemplate")
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var bluetoothManager: BluetoothManager
    private lateinit var bluetoothAdapter: BluetoothAdapter
    private lateinit var bluetoothLeScanner: BluetoothLeScanner
    private lateinit var context: Context
    private var bluetoothGatt: BluetoothGatt? = null

    companion object {
        // IF MODE IS CHANGED, EDIT THIS !!!!!
        private var BT_MODE = "BLE"
//        private var BT_MODE = "CLASSIC"


        // TARGET
        private const val TARGET_MAC: String = "F4:4E:FC:00:00:01" // CB01 US301B PUBLIC
//        private val TARGET_MAC: String = "CB:4E:FC:00:00:01" // CB01 US301B STATIC
//        private val TARGET_MAC: String = "0C:AE:B0:AC:D9:74" // EDIFIER BLE
//        private val TARGET_MAC: String = "AC:90:85:0B:77:C2" // AIRPODS
//        private val TARGET_MAC: String = "BC:1D:89:11:57:B8" // MOTO X30

        // BLE
        private var scanning = false
        private val handler = Handler()

        // CLASSIC
        var myUUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
        var mBluetoothSocket: BluetoothSocket? = null
        var isBlueConnected: Boolean = false
        private const val SCAN_PERIOD: Long = 6000000
    }

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

        val permissions = arrayOf(
            Manifest.permission.BLUETOOTH_SCAN,
            Manifest.permission.BLUETOOTH,
            Manifest.permission.BLUETOOTH_ADMIN,
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.BLUETOOTH_CONNECT,
            Manifest.permission.BLUETOOTH_PRIVILEGED,
        )
        requestPermissions(permissions, 100)
        checkSelfPermission()

        bluetoothManager = this.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothAdapter = bluetoothManager.adapter

        lifecycleScope.launch {
            withContext(Dispatchers.Main) {
                HomeViewModel.signalCallConnect.observe(this@MainActivity) { mode ->
                    when (mode) {
                        "BLE" -> {
                            // BLE MODE
                            startBLE()
                        }
                        "CLASSIC" -> {
                            // CLASSIC MODE
                            registerReceiver(receiverBR, IntentFilter(BluetoothDevice.ACTION_FOUND))
                            bluetoothAdapter.startDiscovery()
                        }
                        else -> {}
                    }
                }
            }
        }
    }

    // BLE MODE FUNCTION
    @RequiresApi(Build.VERSION_CODES.S)
    private fun startBLE() {
        bluetoothLeScanner = bluetoothManager.adapter.bluetoothLeScanner
        checkSelfPermission()
        Log.wtf("[CCMETA]", "bluetoothLeScanner.startScan(mScanCallback)")

        if (!scanning) { // Stops scanning after a pre-defined scan period.
            handler.postDelayed({
                scanning = false
                bluetoothLeScanner.stopScan(leScanCallback)
                cc("bluetoothLeScanner.stopScan(leScanCallback)")
            }, SCAN_PERIOD)
            scanning = true
            bluetoothLeScanner.startScan(leScanCallback)
        } else {
            scanning = false
            bluetoothLeScanner.stopScan(leScanCallback)
        }

        //clock
        thread {
            while (false) {
                Thread.sleep(1000)
                cc("System.nanoTime:" + System.nanoTime().toString())
            }
        }
    }

    // BLE MODE CALLBACK
    private val leScanCallback = object : ScanCallback() {
        override fun onScanFailed(errorCode: Int) {
            super.onScanFailed(errorCode)
            Log.wtf("[CCMETA]", "onScanFailed")
        }

        @RequiresApi(Build.VERSION_CODES.S)
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            checkSelfPermission()
            super.onScanResult(callbackType, result)
            val device = result?.device
            if (device?.name != null && device.name.isNotEmpty()) {

                cc("BLE MODE:" + device.address + "|" + device.name)

                if (TARGET_MAC != device.address) return
                if (!result.isConnectable) {
                    Log.wtf("[CCMETA] isConnectable", result.isConnectable.toString())
                    return
                }

                // GATT CONNECT MODE DIFF WITH CLASSIC
                if (bluetoothGatt != null) {
                    bluetoothGatt!!.disconnect()
                    bluetoothGatt!!.close()
                    bluetoothGatt = null
                }

                bluetoothGatt = device.connectGatt(context, true, object : BluetoothGattCallback() {

                    override fun onConnectionStateChange(
                        gatt: BluetoothGatt, status: Int, newState: Int
                    ) {
                        super.onConnectionStateChange(gatt, status, newState)
                        cc("onConnectionStateChange status:" + status.toString())
                        cc("onConnectionStateChange newState:" + newState.toString())
                        checkSelfPermission()


//                        bluetoothAdapter.getRemoteDevice(device.address)
                        if (!gatt.discoverServices()) {
                            cc("gatt.discoverServices is failed")
                            return
                        }

                        // Modify UI
                        lifecycleScope.launch {
                            withContext(Dispatchers.Main) {
                                HomeViewModel.setConnectDeviceInfo(
                                    gatt.device.address,
                                    (newState == BluetoothProfile.STATE_CONNECTED).toString(),
                                    BT_MODE,
                                )
                            }
                        }
                    }

                    override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
                        cc("[CCMETA] onServicesDiscovered ")
                        super.onServicesDiscovered(gatt, status)
                        cc("gatt.services size " + gatt.services.size)
                        for (i in gatt.services.indices) {
                            cc("service No.$i :" + gatt.services[i].uuid.toString())
                        }

                        val service = gatt.getService(gatt.services[0].uuid)
                        checkSelfPermission()

                        cc("characteristics.size:" + service.characteristics.size)
                        service.characteristics.forEach { i ->
                            // fuck this place  properties always 20
                            i.uuid.leastSignificantBits
                            cc("characteristic uuid:" + i.uuid.toString())
                            cc("characteristic properties:" + i.properties.toString())
                            cc("characteristic permissions:" + i.permissions.toString())
                            cc("characteristic writeType:" + i.writeType.toString())
                            when (i.properties) {
                                0x92 -> {
                                    cc("gatt.readCharacteristic i.uuid:" + i.uuid.toString())
                                    gatt.setCharacteristicNotification(i, true)
                                    i.descriptors.forEach { descriptor ->
                                        descriptor.value =
                                            BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                                        gatt.writeDescriptor(descriptor)
                                    }
                                }
                                0x0C -> {
                                    lifecycleScope.launch {
                                        withContext(Dispatchers.Main) {
                                            HomeViewModel.signalCallOff.observe(this@MainActivity) { item: String ->
                                                if (item == "OFF") {
                                                    i.value = "OFF\n".toByteArray()
                                                    gatt.writeCharacteristic(i)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    override fun onCharacteristicRead(
                        gatt: BluetoothGatt?,
                        characteristic: BluetoothGattCharacteristic?,
                        status: Int
                    ) {
                        cc("onCharacteristicRead:")
                        super.onCharacteristicRead(gatt, characteristic, status)
                        if (status == BluetoothGatt.GATT_SUCCESS) {
                            cc("onCharacteristicRead characteristic uuid:" + characteristic?.uuid.toString())
                            cc("onCharacteristicRead characteristic?.value.size:" + characteristic?.value?.size)
                            cc("onCharacteristicRead characteristic?.value:" + characteristic?.value?.decodeToString())
                        } else {
                            cc("status:" + status.toString())
                        }
                    }

                    override fun onCharacteristicWrite(
                        gatt: BluetoothGatt?,
                        characteristic: BluetoothGattCharacteristic?,
                        status: Int
                    ) {
                        cc("onCharacteristicWrite")
                        super.onCharacteristicWrite(gatt, characteristic, status)
                    }

                    override fun onCharacteristicChanged(
                        gatt: BluetoothGatt?, characteristic: BluetoothGattCharacteristic?
                    ) {
                        cc("onCharacteristicChanged")
                        super.onCharacteristicChanged(gatt, characteristic)
                        val readText = characteristic?.value?.decodeToString() ?: ""
                        lifecycleScope.launch {
                            withContext(Dispatchers.Main) {
                                HomeViewModel.setReceiveMessage(readText)
                            }
                        }
                    }

                    override fun onDescriptorRead(
                        gatt: BluetoothGatt?, descriptor: BluetoothGattDescriptor?, status: Int
                    ) {
                        super.onDescriptorRead(gatt, descriptor, status)
                        cc("onDescriptorRead")
                    }

                    override fun onDescriptorWrite(
                        gatt: BluetoothGatt?, descriptor: BluetoothGattDescriptor?, status: Int
                    ) {
                        cc("onDescriptorWrite ")
                        super.onDescriptorWrite(gatt, descriptor, status)
                    }

                }, TRANSPORT_LE) // THIS IS MUST NOT AUTO, FIX THE STATUS=133 ON 230324
                cc("bluetoothGatt?.device?.bondState:" + bluetoothGatt?.device?.bondState.toString())
            }
        }
    }

    // CLASSIC MODE  Create a BroadcastReceiver for ACTION_FOUND.
    private val receiverBR = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            when (intent.action) {
                BluetoothDevice.ACTION_FOUND -> {
                    checkSelfPermission()
                    val device: BluetoothDevice =
                        intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE) ?: return
                    cc("CLASSIC MODE:" + device.address + "|" + device.name)

                    if (device.address == TARGET_MAC) {
                        startClassicBT(device)
                    }
                }
            }
        }
    }

    // CLASSIC MODE FUNCTION
    private fun startClassicBT(device: BluetoothDevice) {
        thread {
            checkSelfPermission()
            bluetoothAdapter.cancelDiscovery()

            if (mBluetoothSocket != null && isBlueConnected) finish()

            Log.wtf("[CCMETA]", "startClassicBT thread")

            val uuids = device.uuids
            var uuidsString = ""
            if (uuids != null) {
                for (uuid in uuids) {
                    cc(uuid.toString())
                    uuidsString += "$uuid "
                }
            }
            val mBluetoothSocket: BluetoothSocket by lazy(LazyThreadSafetyMode.NONE) {
                device.createRfcommSocketToServiceRecord(myUUID)
            }
            Log.wtf("[CCMETA] isConnected", mBluetoothSocket.isConnected.toString())
            if (!mBluetoothSocket.isConnected) {
                cc("mBluetoothSocket.use { socket -> socket.connect() }")
                mBluetoothSocket.use { socket ->
                    socket.connect()
                    if (!socket.isConnected) {
                        cc("socket is fucked:337")
                        return@thread
                    }
                    lifecycleScope.launch {
                        withContext(Dispatchers.Main) {
                            HomeViewModel.setConnectDeviceInfo(
                                device.address, socket.isConnected.toString(), mode = "CLASSIC"
                            )
                        }
                    }
                    val outputStream = mBluetoothSocket.outputStream
                    val inputStream = mBluetoothSocket.inputStream
                    val bufferRead = ByteArray(128)
                    val bufferWrite = "OFF\n".toByteArray()

                    fun sendToRemote(): Boolean {
                        return try {
                            outputStream.write(bufferWrite)
                            true
                        } catch (e: Exception) {
                            cc("Input stream was disconnected at outputStream.write")
                            false
                        }
                    }

                    lifecycleScope.launch {
                        withContext(Dispatchers.Main) {
                            HomeViewModel.signalCallOff.observe(this@MainActivity) { item: String ->
                                if (item == "OFF") {
                                    sendToRemote()
                                    cc("sendToRemote")
                                }
                            }
                        }
                    }

                    // while for read socket buffer from bluetooth device
                    while (socket.isConnected) {

                        try {
                            inputStream.read(bufferRead)
                        } catch (e: Exception) {
                            cc("Input stream was disconnected at inputStream.read")
                            return@thread
                        }
                        val readText =
                            bufferRead.filter { byte -> byte.toInt() != 0x00 }.toByteArray()
                                .decodeToString()
                        cc("socket readText = $readText")
                        lifecycleScope.launch {
                            withContext(Dispatchers.Main) {
                                HomeViewModel.setReceiveMessage(readText)
                            }
                        }
                    }
                    cc("SOCKET DISCONNECTED")
                }
            }

        }
    }

    // LOG TOOL
    private fun cc(text: String?) {
        Log.wtf("[CCMETA]", text)
    }

    // PERMISSION
    private fun checkSelfPermission() {
        if (ActivityCompat.checkSelfPermission(
                this, Manifest.permission.BLUETOOTH_ADMIN
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            cc("checkSelfPermission BLUETOOTH_ADMIN failed")
            return
        }
    }
}

