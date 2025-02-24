# KNX IoT on NXP hardware Shell help

There are several implemented commands for KNX application support in the shell. These commands are added to the OpenThread CLI shell using OpenThread APIs. The available commands can be checked by entering `help` in the CLI:

```
> help
...
knx_got
knx_ia
knx_iid
knx_fid
knx_factoryreset
knx_pm
Done
>
```

The following 

- individual address (ia). This can be set by issuing the `knx_ia` command from CLI.
```
> knx_ia
Device individual address: 65535
Done
> knx_ia 1
Done
> knx_ia
Device individual address: 1
Done
```

- installation id (iid). This can be set by issuing the `knx_iid` command from CLI.
```
> knx_iid
Device installation id: 0
Done
> knx_iid 1
iid set:
1
Done
> knx_iid
Device installation id: 1
Done
>
```

- fabric identifier (fid). This can be set by issuing the `knx_fid` command from CLI.
```
> knx_fid
Fabric identifier: 0
Done
> knx_fid 123
Done
> knx_fid
Fabric identifier: 123
Done
>
```

- group object table (got). The available operations of this commands are `add`, `show` and `remove`.

The `knx_got add` command allows the user to add a new group object table entry.

```
> knx_got add 0 /p/o_1_1 20 ga 1
oc_register_group_multicasts: mport 0

 oc_register_group_multicasts index=0 i=0 group: 1  cflags=
  oc_create_multicast_group_address_with_port S=2 iid=lu G=1 B4=0 B3=1 B2=0 B1=0


coap://[ff32:0030:fd00:0000:0001:0000:0000:0001]:0
  oc_create_multicast_group_address_with_port S=5 iid=lu G=1 B4=0 B3=1 B2=0 B1=0


coap://[ff35:0030:fd00:0000:0001:0000:0000:0001]:0
Done
```

The parameters for the command above (`knx_got add *id* *uri_path* *cflags* ga *group_addresses*`) are the following
- `id` represents the id of the entry in the group object table. This needs to be of integer value.
- `uri_path` represents the registered CoAP uri_path used for the demo communication. This needs to be a character string. For example, the applications have `/p/o_1_1` registered in the CoAP resources and this needs to be set from CLI also.
- `cflags` represents the communication flags. This parameter represents a binary encoding entry in the group object table that lets the KNX stack know how to threat the KNX message. The encoding is the following:
    - OC_CFLAG_COMMUNICATION = 1 << 2, /**< false = Group Object value cannot read or written.*/
    - OC_CFLAG_READ = 1 << 3, /**< 8 false = Group Object value cannot be read.*/
    - OC_CFLAG_WRITE = 1 << 4, /**< 16 false = Group Object value cannot be written.*/
    - OC_CFLAG_INIT = 1 << 5, /**< 32 false = Disable read after initialization.*/
    - OC_CFLAG_TRANSMISSION = 1 << 6, /**< 64 false = Group Object value is not transmitted.*/
    - OC_CFLAG_UPDATE = 1 << 7, /**< 128 false = Group Object value is not updated.*/

For example, when sending out the CoAP message to toggle the LED, a CoAP POST message is sent, that needs to update the value on the remote node, so the OC_CFLAG_WRITE bit needs to be set, along with the OC_CFLAG_COMMUNICATION bit, corresponding to an integer value of 20 (0b0001 0100).
- `group_addresses` represents the group addresses entry. Multiple entries are supported. For example, to add a GOT entry for group addresses 1, 2, 5, 7, 23, 123, user needs to enter command:
```
> knx_got add 0 /p/o_1_1 20 ga 1 2 5 7 23 123
```

The `knx_got show` command allows the user to view the available entries in the Group Object Table.

```
> knx_got show
Showing Group Object Table
    id (0)     : 0

    href (11)  : /p/o_1_1

    cflags (8) : 20 string:
    ga (7)     : [
 1
 ]
Done
>
```
This allows the user to see all available entries parameters set on the `knx_got add` command.

The `knx_got remove` command allows the user to remove an entry in the Group Object Table. This entry is based on the `id` used when adding the entry. For example, when adding an entry with command `knx_got add 0 /p/o_1_1 20 ga 1`, the `id` is set to 0 and the entry can be removed using command `knx_got remove 0`. The success of the command can be checked with the `knx_got show` command.

```
> knx_got remove 0
Done
> knx_got show
Showing Group Object Table
Done
>
```

- KNX factoryreset command, done by issuing `knx_factoryreset` command from CLI.
This command is used to call KNX storage reset for device 0 (this device) and code 2 (Factory Reset to default state). It also erases the OpenThread Stack persistent data and causes the board to reset.
```
> knx_factoryreset
factory_presets_cb: resetting device
Deleting Group Object Table from Persistent storage
Deleting Group Recipient Table from Persistent storage
Deleting Group Publisher Table from Persistent storage
...
```

- programming mode (om). This can be set by issuing the `knx_pm` command from CLI.
```
> knx_pm
Device in programming mode: FALSE
Done
> knx_pm 1
Done
> knx_pm
Device in programming mode: TRUE
Done
> knx_pm 0
Done
> knx_pm
Device in programming mode: FALSE
Done
```