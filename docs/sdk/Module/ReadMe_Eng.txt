Nikon Type0025 Module SDK Revision.7 summary


Usage
 Control the camera.


Supported camera
 Z 50


Environment of operation
 [Windows]
    Windows 10 64bit edition
    Windows 11 64bit edition
    * You must install "microsoft visual c++ redistributable package for visual studio 2017"

 [Macintosh]
    macOS 12.5(Monterey)
    macOS 13(Ventura)
    macOS 14(Sonoma)
    * 64bit mode only (32bit mode is not supported)


Contents
 [Windows]
    Documents
      MAID3(E).pdf : Basic interface specification
      MAID3Type0025(E).pdf : Extended interface specification used 
                                                              by Type0025 Module
      Usage of Type0025 Module(E).pdf : Notes for using Type0025 Module
      Type0025 Sample Guide(E).pdf : The usage of a sample program

    Binary Files
      Type0025.md3 : Type0025 Module for Win
      NkdPTP.dll : Driver for PTP mode used by Win
      dnssd.dll : Driver for PTP mode used by Win
      NkRoyalmile.dll : Driver for PTP mode used by Win

    Header Files
      Maid3.h : Basic header file of MAID interface
      Maid3d1.h : Extended header file for Type0025 Module
      NkTypes.h : Definitions of the types used in this program
      NkEndian.h : Definitions of the types used in this program
      Nkstdint.h : Definitions of the types used in this program

    Sample Program
      Type0025CtrlSample(Win) : Project for Microsoft Visual Studio 2017

 [Macintosh]
    Documents
      MAID3(E).pdf : Basic interface specification
      MAID3Type0025(E).pdf : Extended interface specification used
                                                             by Type0025 Module
      Usage of Type0025 Module(E).pdf : Notes for using Type0025 Module
      Type0025 Sample Guide(E).pdf : The usage of a sample program
      [Mac OS] Notice about using Module SDK(E).txt : Notes for using SDK
                                                                on Mac OS

    Binary Files
      Type0025 Module.bundle : Type0025 Module for Mac
      libNkPTPDriver2.dylib : PTP driver for Mac 
      Royalmile.framework : PTP driver for Mac

    Header Files
      Maid3.h : Basic header file of MAID interface
      Maid3d1.h : Extended header file for Type0025 Module
      NkTypes.h : Definitions of the types used in this program
      NkEndian.h : Definitions of the types used in this program
      Nkstdint.h : Definitions of the types used in this program

    Sample Program
      Type0025CtrlSample(Mac) : Sample program project for Xcode 13.2.1(BaseSDK : macOS)


Limitations
 This module cannot control two or more cameras.

Cautions
 When using the New SDK,  dnssd.dll and NkRoyalmile.dll are required.
 Please refer the following document and notice.
 - Type0025 Sample Guide(E).pdf
  "Files - Windows"
