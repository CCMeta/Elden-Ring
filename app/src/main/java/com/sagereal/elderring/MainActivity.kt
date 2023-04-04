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
        private var BT_MODE = ""
//        private var BT_MODE = "CLASSIC"


        // TARGET
//        private const val TARGET_MAC: String = "00:1D:29:B7:7B:FB" // DORO3500
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
        bluetoothLeScanner = bluetoothManager.adapter.bluetoothLeScanner

        lifecycleScope.launch {
            withContext(Dispatchers.Main) {
                HomeViewModel.signalCallConnect.observe(this@MainActivity) { mode ->
                    BT_MODE = mode
                    bluetoothLeScanner.stopScan(leScanCallback)
                    bluetoothGatt?.disconnect()
                    bluetoothGatt?.close()
                    bluetoothGatt = null
                    mBluetoothSocket?.close()
                    when (BT_MODE) {
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
        checkSelfPermission()
        Log.wtf("[CCMETA]", "bluetoothLeScanner.startScan(mScanCallback)")

//        if (!scanning) { // Stops scanning after a pre-defined scan period.
//            handler.postDelayed({
//                scanning = false
//                bluetoothLeScanner.stopScan(leScanCallback)
//                _log("bluetoothLeScanner.stopScan(leScanCallback)")
//            }, SCAN_PERIOD)
//            scanning = true
//            bluetoothLeScanner.startScan(leScanCallback)
//        } else {
//            scanning = false
//            bluetoothLeScanner.stopScan(leScanCallback)
//        }
        bluetoothLeScanner.startScan(leScanCallback)


        //clock for console
        thread {
            while (false) {
                Thread.sleep(1000)
                _log("System.nanoTime:" + System.nanoTime().toString())
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

                _log("BLE MODE:" + device.address + "|" + device.name)

                if (TARGET_MAC != device.address) return
                if (!result.isConnectable) {
                    Log.wtf("[CCMETA] isConnectable", result.isConnectable.toString())
                    return
                }

                // GATT CONNECT MODE DIFF WITH CLASSIC
                if (bluetoothGatt != null && bluetoothGatt!!.device.address == device.address) {
                    return
                }

                bluetoothGatt = device.connectGatt(context, true, object : BluetoothGattCallback() {

                    override fun onConnectionStateChange(
                        gatt: BluetoothGatt, status: Int, newState: Int
                    ) {
                        super.onConnectionStateChange(gatt, status, newState)
                        _log("onConnectionStateChange status:" + status.toString())
                        _log("onConnectionStateChange newState:" + newState.toString())
                        checkSelfPermission()

//                        bluetoothAdapter.getRemoteDevice(device.address)
                        if (!gatt.discoverServices()) {
                            _log("gatt.discoverServices is failed")
                            return
                        }

                        // Modify UI
                        lifecycleScope.launch {
                            withContext(Dispatchers.Main) {
                                HomeViewModel.setConnectDeviceInfo(
                                    gatt.device.address,
                                    (newState == BluetoothProfile.STATE_CONNECTED),
                                    BT_MODE,
                                )
                            }
                        }
                    }

                    override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
                        _log("[CCMETA] onServicesDiscovered ")
                        super.onServicesDiscovered(gatt, status)
                        _log("gatt.services size " + gatt.services.size)
                        for (i in gatt.services.indices) {
                            _log("service No.$i :" + gatt.services[i].uuid.toString())
                        }

                        val service = gatt.getService(gatt.services[0].uuid)
                        checkSelfPermission()

                        _log("characteristics.size:" + service.characteristics.size)
                        service.characteristics.forEach { i ->
                            // fuck this place  properties always 20
                            i.uuid.leastSignificantBits
                            _log("characteristic uuid:" + i.uuid.toString())
                            _log("characteristic properties:" + i.properties.toString())
                            _log("characteristic permissions:" + i.permissions.toString())
                            _log("characteristic writeType:" + i.writeType.toString())
                            when (i.properties) {
                                0x92 -> {
                                    _log("gatt.readCharacteristic i.uuid:" + i.uuid.toString())
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
                        _log("onCharacteristicRead:")
                        super.onCharacteristicRead(gatt, characteristic, status)
                        if (status == BluetoothGatt.GATT_SUCCESS) {
                            _log("onCharacteristicRead characteristic uuid:" + characteristic?.uuid.toString())
                            _log("onCharacteristicRead characteristic?.value.size:" + characteristic?.value?.size)
                            _log("onCharacteristicRead characteristic?.value:" + characteristic?.value?.decodeToString())
                        } else {
                            _log("status:" + status.toString())
                        }
                    }

                    override fun onCharacteristicWrite(
                        gatt: BluetoothGatt?,
                        characteristic: BluetoothGattCharacteristic?,
                        status: Int
                    ) {
                        _log("onCharacteristicWrite")
                        super.onCharacteristicWrite(gatt, characteristic, status)
                    }

                    override fun onCharacteristicChanged(
                        gatt: BluetoothGatt?, characteristic: BluetoothGattCharacteristic?
                    ) {
                        _log("onCharacteristicChanged")
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
                        _log("onDescriptorRead")
                    }

                    override fun onDescriptorWrite(
                        gatt: BluetoothGatt?, descriptor: BluetoothGattDescriptor?, status: Int
                    ) {
                        _log("onDescriptorWrite ")
                        super.onDescriptorWrite(gatt, descriptor, status)
                    }

                }, TRANSPORT_LE) // THIS IS MUST NOT AUTO, FIX THE STATUS=133 ON 230324
                _log("bluetoothGatt?.device?.bondState:" + bluetoothGatt?.device?.bondState.toString())
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
                    _log("CLASSIC MODE:" + device.address + "|" + device.name)

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

            Log.wtf("[CCMETA]", "Run startClassicBT thread")
            device.fetchUuidsWithSdp()
            val uuids = device.uuids
            var uuidsString = ""
            if (uuids != null) {
                for (uuid in uuids) {
                    _log(uuid.toString())
                    uuidsString += "$uuid "
                }
                lifecycleScope.launch{
                    withContext(Dispatchers.Main){
                        HomeViewModel.textMessage.postValue(uuidsString)
                    }
                }
            }
            val localBluetoothSocket: BluetoothSocket by lazy(LazyThreadSafetyMode.NONE) {
                device.createRfcommSocketToServiceRecord(myUUID)
            }
            mBluetoothSocket = localBluetoothSocket
            Log.wtf("[CCMETA] isConnected", localBluetoothSocket.isConnected.toString())
            if (!localBluetoothSocket.isConnected) {
                _log("mBluetoothSocket.use { socket -> socket.connect() }")
                localBluetoothSocket.use { socket ->
                    socket.connect()
                    if (!socket.isConnected) {
                        _log("socket is fucked:337")
                        return@thread
                    }
                    lifecycleScope.launch {
                        withContext(Dispatchers.Main) {
                            HomeViewModel.setConnectDeviceInfo(
                                device.address, socket.isConnected, mode = "CLASSIC"
                            )
                        }
                    }
                    val outputStream = localBluetoothSocket.outputStream
                    val inputStream = localBluetoothSocket.inputStream
                    val bufferRead = ByteArray(128)
                    val textOff = "OFF\n"

                    fun writeOutputStream(text: String): Boolean {
                        return try {
                            outputStream.write(text.toByteArray())
                            true
                        } catch (e: Exception) {
                            _log("Input stream was disconnected at outputStream.write")
                            false
                        }
                    }

                    lifecycleScope.launch {
                        withContext(Dispatchers.Main) {
                            HomeViewModel.signalCallOff.observe(this@MainActivity) { item: String ->
                                if (item == "OFF") {
                                    writeOutputStream(textOff)
                                    _log("sendToRemote")
                                }
                            }
                        }
                    }

                    // while for read socket buffer from bluetooth device
                    while (socket.isConnected) {
                        try {
                            inputStream.read(bufferRead)
                        } catch (e: Exception) {
                            _log("Input stream was disconnected at inputStream.read")
                            break
                        }
                        val readText =
                            bufferRead.filter { byte -> byte.toInt() != 0x00 }.toByteArray()
                                .decodeToString()
                        _log("socket readText = $readText")
                        lifecycleScope.launch {
                            withContext(Dispatchers.Main) {
                                HomeViewModel.setReceiveMessage(readText)
                            }
                        }
                    }
                    if (!socket.isConnected) {
                        lifecycleScope.launch {
                            withContext(Dispatchers.Main) {
                                HomeViewModel.setConnectDeviceInfo(
                                    "NONE", socket.isConnected, "NONE"
                                )
                            }
                        }
                        mBluetoothSocket = null
                        _log("SOCKET DISCONNECTED")
                    }
                }
            }

        }
    }

    // LOG TOOL
    private fun _log(text: String?) {
        Log.wtf("[CCMETA]", text)
    }

    // PERMISSION
    private fun checkSelfPermission() {
        if (ActivityCompat.checkSelfPermission(
                this, Manifest.permission.BLUETOOTH_ADMIN
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            _log("checkSelfPermission BLUETOOTH_ADMIN failed")
            return
        }
    }
}

