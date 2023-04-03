package com.sagereal.elderring.ui.home

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import androidx.fragment.app.Fragment
import com.sagereal.elderring.databinding.FragmentHomeBinding

class HomeFragment : Fragment() {

    private var _binding: FragmentHomeBinding? = null
    private val binding get() = _binding!!

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        val homeViewModel = HomeViewModel

        _binding = FragmentHomeBinding.inflate(inflater, container, false)
        val root: View = binding.root
        binding.vm = homeViewModel

        val textMessage = binding.textMessage
        homeViewModel.textMessage.observe(viewLifecycleOwner) {
            textMessage.text = it
        }

        val textMac = binding.textMac
        homeViewModel.textMac.observe(viewLifecycleOwner) {
            textMac.text = it
        }
        val textMode = binding.textMode
        homeViewModel.textMode.observe(viewLifecycleOwner) {
            textMode.text = it
        }
        val textState = binding.textState
        homeViewModel.textState.observe(viewLifecycleOwner) {
            textState.text = it
        }


        val buttonSendOff: Button = binding.buttonSendOff
        homeViewModel.buttonSendOffEnableState.observe(viewLifecycleOwner) {
            buttonSendOff.isEnabled = it
        }
        buttonSendOff.setOnClickListener {
            HomeViewModel.signalCallOff.postValue("OFF")
        }

        val connectBle = binding.connectBle
        connectBle.setOnClickListener {
            homeViewModel.signalCallConnect.postValue("BLE")
        }

        val connectBr = binding.connectBr
        connectBr.setOnClickListener {
            homeViewModel.signalCallConnect.postValue("CLASSIC")
        }


        return root
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}