# KNX IoT demo using two boards with NXP OpenThread Border Router

This demo requires an additional board acting as OpenThread border Router (OTBR). The device used in the demo is [NXP FRDM-RW612](https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-RW612).

A combination of the two previous demos, the [simple demo between two MCXW71 boards](knx_demo_nxp.md) and the [demo including an OTBR and a PC](knx_demo_nxp_otbr.md) can be showcased by using the two buttons available on the FRDM-MCXW71 board, marked SW2 and SW4. The first button will trigger a KNX communication using the `/p/o_1_1' uri path, while the other button will communicate on KNX using '/p/o_1_2' uri path.

Shown in the figure below is the case of a PC running the Light Switched Actuator Basic (LSAB) application from the KNX IoT repository modified to support multicast communication from the KNX light switched sensor basic (LSSB) application running on the NXP FRDM-MCXW71 in the Thread network. The PC is connected to a WiFi network and the link between the WiFi and Thread realms is done using a NXP FRDM-RW612 running the OpenThread Border Router firmware. There is also another NXP FRDM-MCXW71 board running the light switched actuator basic (LSAB) application.

![](images/two_knx_boards_otbr_pc_demo.jpg)

The PC and the OTBR are configured the same as in the previous [demo](knx_demo_nxp_otbr.md), including the WiFi and OpenThread network credentials.

The Thread commissioning to the network created by the NXP OTBR is done as before, by issuing the following commands on the two MCXW71 boards:

```
dataset init new

dataset channel 17

dataset networkkey 00112233445566778899aabbccddeeff

dataset panid 0xabcd

dataset commit active

ifconfig up

thread start
```
For help with KNX commands, see [here](knx_shell_help.md).

The KNX commands needed to be entered on the light sensor board are:

```
knx_ia 1

knx_iid 1

knx_got add 0 /p/o_1_1 20 ga 1

knx_got add 1 /p/o_1_2 20 ga 10
```

The KNX commands needed to be entered on the light actuator board are:

```
knx_ia 2

knx_iid 1

knx_got add 1 /p/o_1_2 20 ga 10
```

After these settings are done, user can check the functionality of the KNX protocol by pressing SW4 on the light sensor board and observing the monochrome LED on the light actuator board marked D1 toggling the blue light on and off. Pressing SW2 on the light sensor board will trigger the same output on the PC as in the previous KNX with OTBR demo.