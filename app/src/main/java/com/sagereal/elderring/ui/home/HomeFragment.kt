package com.sagereal.elderring.ui.home

import android.annotation.SuppressLint
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
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

        val textState131 = binding.textState131
        homeViewModel.textState131.observe(viewLifecycleOwner) {
            textState131.text = it
        }
        val textState110 = binding.textState110
        homeViewModel.textState110.observe(viewLifecycleOwner) {
            textState110.text = it
        }
        val textMac = binding.textMac
        homeViewModel.textMac.observe(viewLifecycleOwner) {
            textMac.text = it
        }
        val textState = binding.textState
        homeViewModel.textState.observe(viewLifecycleOwner) {
            textState.text = it
        }

        val buttonSendOff: Button = binding.buttonSendOff

        buttonSendOff.setOnClickListener {
            onClickButtonSendOff(binding)
        }

        return root
    }

    @SuppressLint("SetTextI18n")
    private fun onClickButtonSendOff(binding: FragmentHomeBinding) {
        //send off to bluetooth
        val homeViewModel = HomeViewModel
        Log.wtf("[CCMETA]", "onClickButtonSendOff")
        binding.textState110.text = "" + homeViewModel.textState110.value + "some 110 msg"
        binding.textState131.text = "" + homeViewModel.textState131.value + "some 131 msg"
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}