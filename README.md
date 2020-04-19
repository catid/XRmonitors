# XRmonitors

Provides Virtual Monitors on Microsoft Windows 10 Mixed Reality Headsets such as the HP Reverb.

Key product features:

- Is an essential add-on for the HP Reverb headset
- Allows you to use your familiar keyboard, mouse, tablet, and applications
- Pass-through cameras allow you to see and interact with the world as normal
- Allows you to work with privacy and focus on the task at hand without distractions
- Bring multiple-monitor setups anywhere with you with your laptop
- Works with MacBook and Windows laptops without GPUs
- Allows the user to recline while using their computer
- Full access to multiple monitors while on a couch or lawn chair - Solves back strain!
- Places content at a 1.34 meter distance from the user for eye comfort
- Comfortable for everyone: No vergence-accommodation conflict
- 2x the distance of normal monitors - Solves eye strain!
- Cheaply add Extra, Private 4K monitors by attaching HDMI dongle
- Perfectly color-corrected displays for designers
- Allows the user to work outside on sunny days
- Private monitors for viewing sales projections, financial statements, personal correspondence
- Allows the user to customize the physical size, DPI of their monitor
- Finally, no bezel between side by side monitors
- Blue light filter mode


## Install Instructions

Plug in your HP Reverb headset.

Verify that the Windows Mixed Reality Portal environment works.

Double-click the installer and accept our certificate.

You are encouraged to install the Windows Mixed Reality OpenXR Runtime from the Microsoft Store.
You may also need to upgrade your version of Windows 10 to get VR features.



## Build Instructions

Requires: Visual Studio 2019

Clone repo with git: 
git clone https://github.com/catid/XRmonitors.git
cd XRmonitors
git submodule update --init --recursive

Generate your own private key to sign the installer and place it under signing/DigiCertPrivateKey.pfx

Open XRmonitors.sln with Visual Studio 2019
Select build mode: Release x64
Build Menu > Rebuild Solution

Setup executable is written to bin/XRmonitorsSetup.exe


## Debug Instructions

Run the installed XRmonitors UI application but click so "XR Monitors Disabled" is shown in red.

Build the Visual Studio project in Debug mode and set XRmonitorsHologram C++ project as the Startup Project.

Press F5 to run in debug mode.

You can use the same approach to build the XRmonitorsUI.
