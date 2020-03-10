Step-by-step guide
Ensure the snmp package are installed in the chosen system: 
  For a Ubuntu machine: sudo apt-get install snmp.
  For a CentOS machine: yum -y install net-snmp-utils.
Ensure the WIENER-CRATE-MIB file is present in the system at the right directory:
  For a Ubuntu machine: /usr/share/snmp/mib or /home/.snmp/mibs/
  For a CentOS machine: /usr/share/snmp/mibs/
Ensure that the MPOD is registered on your DHCP server and get its ip address. (Alternatively at Desy for direct connection: Ensure the PC is connected to the MPOD Wiener Power Supply through a Ethernet cable. You configure the connection for a Link-Local Only. This is done thought  the Network Manager at the iPv4 Settings.)
Open a web-browser and tap its IP address; (169.254.1.240 at DESY only!)
Ensure you have execute rights on the different scripts you want to use; they are on github: percivalui/user_scripts/wiener
To check, open a terminal, navigate to the folder where the scripts are and type ll.
If the rights have not been granted, type: chmod +rwx script_name.sh (for the different scripts you want to use). 
Ensure the IP address in the scripts is correct; type grep WIENER_IP= and change it if necessary.
Run the different scripts in the terminal in the appropriate order (remember to change the IP according to your configuration!)
./Carrier_UP_fast.sh
./PwB_UP_fast.sh
Operate the system
./PwB_DOWN_fast.sh
./Carrier_DOWN_fast.sh
