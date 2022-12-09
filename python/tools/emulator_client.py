'''
Created on March 10, 2015

@author: ckd27546

'''

import argparse, socket, time, sys

class EmulatorClientError(Exception):
    
    def __init__(self, msg):
        self.msg = msg
        
    def __str__(self):
        return repr(self.msg)


class EmulatorClient(object):
    ''' Test setting IP/MAC addresses in the firmware '''
 
    # Define legal commands #TODO: To include MAC/IP address?
    LegalCommands = {
                     "start"   : "startd\r\n",
                     "command" : "COMAND\n\r",
                     "stop"    : "\x20stopd\r\n"
                     }

    ## New stuff
    Eth_Dev_RW   = 0x00000001
    
    MAC_0_ADDR   = 0x60006006                                       
    MAC_1_ADDR   = 0x60006000                                       
    MAC_2_ADDR   = 0x60006012                                       
    MAC_3_ADDR   = 0x6000600C 
    MAC_4_ADDR   = 0x6000601E                                       
    MAC_5_ADDR   = 0x60006018                                       
    MAC_6_ADDR   = 0x6000602A                                       
    MAC_7_ADDR   = 0x60006024 
    MAC_8_ADDR   = 0x60006036 
    MAC_9_ADDR   = 0x60006030 
    MAC_10_ADDR  = 0x60006042                                       
    MAC_11_ADDR  = 0x6000603C 
    MAC_12_ADDR  = 0x6000604E                                       
    MAC_13_ADDR  = 0x60006048 
    MAC_14_ADDR  = 0x6000605A
    MAC_15_ADDR  = 0x60006054

    IP_0_ADDR    = 0x60000000 
    IP_1_ADDR    = 0x60005000
    IP_2_ADDR    = 0x60000004
    IP_3_ADDR    = 0x60005004                                       
    IP_4_ADDR    = 0x60000008 
    IP_5_ADDR    = 0x60005008   # Fixed address typo from Java client
    IP_6_ADDR    = 0x6000000C                                       
    IP_7_ADDR    = 0x6000500C
    IP_8_ADDR    = 0x60000010       
    IP_9_ADDR    = 0x60005010
    IP_10_ADDR   = 0x60000014
    IP_11_ADDR   = 0x60005014
    IP_12_ADDR   = 0x60000018
    IP_13_ADDR   = 0x60005018
    IP_14_ADDR   = 0x6000001C
    IP_15_ADDR   = 0x6000501C
    
    Num_Req_Imgs = 0x60007000

    (IP, MAC)    = (0, 1)

    def __init__(self):
        
        parser = argparse.ArgumentParser(prog="emulator_client",
                                         description="EmulatorClient - control hardware emulator start, stop & configure node(s)", 
                                         epilog="Specify IP & Mac like: '10.1.0.101:00-07-11-F0-FF-33'")
    
        parser.add_argument('--host', type=str, default='192.168.0.103', 
                            help="select emulator IP address")
        parser.add_argument('--port', type=int, default=4321,
                            help='select emulator IP port')
        parser.add_argument('--timeout', type=int, default=5,
                            help='set TCP connection timeout')
        parser.add_argument('--delay', type=float, default=1.00,
                            help='set delay between each address packet')
        parser.add_argument('--images', type=int,
                            help='Set number of images for hardware to send')
        
        parser.add_argument('--src0', type=str, 
                            help='Configure Mezzanine link 0 IP:MAC addresses')
        parser.add_argument('--src1', type=str, 
                            help='Configure Mezzanine link 1 IP:MAC addresses')
        parser.add_argument('--src2', type=str, 
                            help='Configure Mezzanine link 2 IP:MAC addresses')
        
        parser.add_argument('--dst0', type=str, 
                            help='Configure PC link 0 IP:MAC addresses')
        parser.add_argument('--dst1', type=str, 
                            help='Configure PC link 1 IP:MAC addresses')
        parser.add_argument('--dst2', type=str, 
                            help='Configure PC link 2 IP:MAC addresses')
        
        parser.add_argument('command', choices=EmulatorClient.LegalCommands.keys(), default="start",
                            help="specify which command to send to emulator")
    
        args = parser.parse_args()

        self.host     = args.host 
        self.port     = args.port 
        self.timeout  = args.timeout
        self.delay    = args.delay
        
        if args.images:
            self.images = args.images
        else:
            self.images = None

        if args.src0: 
            self.src0addr = self.extractAddresses(args.src0) 
        else:
            self.src0addr = (None, None)
        if args.src1: 
            self.src1addr = self.extractAddresses(args.src1) 
        else:        
            self.src1addr = (None, None)
        if args.src2: 
            self.src2addr = self.extractAddresses(args.src2) 
        else:        
            self.src2addr = (None, None)
        if args.dst0: 
            self.dst0addr = self.extractAddresses(args.dst0) 
        else:        
            self.dst0addr = (None, None)
        if args.dst1: 
            self.dst1addr = self.extractAddresses(args.dst1) 
        else:        
            self.dst1addr = (None, None)
        if args.dst2: 
            self.dst2addr = self.extractAddresses(args.dst2) 
        else:        
            self.dst2addr = (None, None)

        self.command = args.command       
        if not self.command in EmulatorClient.LegalCommands:
            raise EmulatorClientError("Illegal command %s specified" % self.command)


    def extractAddresses(self, jointAddress):
        ''' Extracting mac/IP from parser argument '''
        delim =  jointAddress.find(":")
        ip    = jointAddress[:delim]
        mac   = jointAddress[delim+1:]
        return (ip, mac)

    def run(self):

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(self.timeout)
        # Open TCP connection
        try:
            self.sock.connect((self.host, self.port))
        except socket.timeout:
            raise EmulatorClientError("Connecting to [%s:%d] timed out" % (self.host, self.port))

        except socket.error, e:
            if self.sock:
                self.sock.close()
            raise EmulatorClientError("Error connecting to [%s:%d]: '%s'" % (self.host, self.port, e))

        if (self.images != None):

            # SNIPPET START #
#             ipList = self.create_ip(src0ip)
#             ipSourceString = ''.join(ipList)
#             self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_0_ADDR, 4, ipSourceString)
#             time.sleep(self.delay)
#             
#             tokenList = self.tokeniser(src0mac)
#             macSourceStr = ''.join(tokenList)
#             self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_0_ADDR, 6, macSourceStr)
            # SNIPPET END #

            # Convert int to byte(s)
            imagesString = '%08X' % (self.images)

            print "User elected to request %d (%s) image(s)." % (self.images, imagesString)

#             print "imagesString, images:", imagesString, self.images
            self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.Num_Req_Imgs, 4, imagesString)
#             print "Done"
            
        elif (self.command == 'start') or (self.command == 'stop'):

            # Transmit command
            try:
                bytesSent = self.sock.send(EmulatorClient.LegalCommands[self.command])
            except socket.error, e:
                if self.sock:
                    self.sock.close()
                raise EmulatorClientError("Error sending %s command: %s" % (self.command, e))
                
            if bytesSent != len(EmulatorClient.LegalCommands[self.command]):
                print "Failed to transmit %s command properly" % self.command
            else:
                print "Transmitted %s command" % self.command
        
        else:
            
            # Configure link(s) 

            src0mac = self.src0addr[EmulatorClient.MAC]
            src1mac = self.src1addr[EmulatorClient.MAC]
            src2mac = self.src2addr[EmulatorClient.MAC]
            dst0mac = self.dst0addr[EmulatorClient.MAC]
            dst1mac = self.dst1addr[EmulatorClient.MAC]
            dst2mac = self.dst2addr[EmulatorClient.MAC]

            src0ip = self.src0addr[EmulatorClient.IP]
            src1ip = self.src1addr[EmulatorClient.IP]
            src2ip = self.src2addr[EmulatorClient.IP]
            dst0ip = self.dst0addr[EmulatorClient.IP]
            dst1ip = self.dst1addr[EmulatorClient.IP]
            dst2ip = self.dst2addr[EmulatorClient.IP]
            
            print "Sending: "
            if src0ip:
                ipList = self.create_ip(src0ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_0_ADDR, 4, ipSourceString)
                time.sleep(self.delay)

            if dst0ip:
                ipList = self.create_ip(dst0ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_1_ADDR, 4, ipSourceString)
                time.sleep(self.delay)
                 
            if src1ip:
                ipList = self.create_ip(src1ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_2_ADDR, 4, ipSourceString)
                time.sleep(self.delay)

            if dst1ip:
                ipList = self.create_ip(dst1ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_3_ADDR, 4, ipSourceString)
                time.sleep(self.delay)
                 
            if src2ip:
                ipList = self.create_ip(src2ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_4_ADDR, 4, ipSourceString)
                time.sleep(self.delay)

            if dst2ip:
                ipList = self.create_ip(dst2ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_5_ADDR, 4, ipSourceString)
                time.sleep(self.delay)

            # IP before Mac now..
            
            if src0mac:
                tokenList = self.tokeniser(src0mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_0_ADDR, 6, macSourceStr)
                time.sleep(self.delay)

            if dst0mac:
                tokenList = self.tokeniser(dst0mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_1_ADDR, 6, macSourceStr)
                time.sleep(self.delay)
     
            if src1mac:
                tokenList = self.tokeniser(src1mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_2_ADDR, 6, macSourceStr)
                time.sleep(self.delay)

            if dst1mac:
                tokenList = self.tokeniser(dst1mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_3_ADDR, 6, macSourceStr)
                time.sleep(self.delay)
     
            if src2mac:
                tokenList = self.tokeniser(src2mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_4_ADDR, 6, macSourceStr)
                time.sleep(self.delay)

            if dst2mac:
                tokenList = self.tokeniser(dst2mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_5_ADDR, 6, macSourceStr)
                time.sleep(self.delay)
#                 print "DEBUGGING macSourceStr '%s'" % macSourceStr
#                 print "DEBUGGING tokenList    ", tokenList

            print "\nWaiting 3 seconds before closing TCP connection.."
            time.sleep(3.0)
            print " Done!"

    def tokeniser(self, string):
        """ Remove white spaces and Split (IP/MAC address) string into list """
        string = string.replace(' ', '')
        if ":" in string:
            tokenList = string.split(":")
        else:
            if "." in string:
                tokenList = string.split(".")
            else:
                if "-" in string:
                    tokenList = string.split("-")
                else:
                    tokenList = string.split()
        return tokenList
        
    def create_ip(self, ip_addr):
        ''' Convert IP address from string format into list '''

        ip_value  = ['0'] * 4        # Return value
        int_value = ['0'] * 4
        var_b = ""

        hexString = ""
        var_i = 0
        tokenList = self.tokeniser(ip_addr)
        for index in range(len(tokenList)):
            var_i = 0

            var_i = int(tokenList[index])
            var_b = (var_i & 0x000000FF)

            hexString = hexString + str(tokenList[index] )
            int_value[index] = var_i
            ip_value[index] = '%02X' % (var_b)  # Hexadecimal conversion
        
        return ip_value     # Returns IP address as a list of integers 
    
    
    # Convert create_mac() into Python
    def create_mac(self, mac_add):
        ''' Convert mac address from string format into list '''
        mac_value = ['0'] * 8
        int_value = ['0'] * 8
        var_b    = [0]
        lenToken = 0
        var_i    = 0
        hdata    = ""
        hexString = ""
        tokenList = self.tokeniser(mac_add)
        for index in range(len(tokenList)):
            token = tokenList[index]
            var_i = 0
            #
            var_i = int(tokenList[index], 16)
            lenToken = len(token)
            var_b = (var_i & 0x000000FF)
            
            for k in range(lenToken):
                hdata = token[k]
                var_b = int(hdata, 16) 
                var_i = var_i + var_b*((lenToken-1-k)*16 + k) 
            
            hexString = hexString + token
            int_value[index] = var_i
            mac_value[index] = str(var_b)

        return mac_value 
    
    def intToByte(self, header, offset, length, offset2, command_b):
        ''' Functionality change so that header inserted into command_b according to offset and offset2 '''
        # header is a list of integers, turn them into 8 digits hexadecimals
        for index in range(len(header)):
            hexNum     = "%x" % header[index]
            hexPadded  = (hexNum).zfill(8)
            byteString = ''.join(chr(int(hexPadded[i:i+2], 16)) for i in range(0, len(hexPadded), 2))
            command_b  = command_b[:8+(index*4)] + byteString + command_b[8+((index+1)*4):]
        return command_b
    
    def send_to_hw(self, Dev_RW, ADDR, Length, data):
        ''' Send (IP/Mac address) to Mezzanine '''
        # Example :
        # Device ID  = 0x40000001;  # HW,  Write
        # Address    = 0x18000004;  # Control Register   
        # length     = 0x00000001;  # Control Register    0x0000 0001
        HEADER = [0,0,0,0]
        HEADER[0] = Dev_RW; HEADER[1] = ADDR;  HEADER[2] = Length;
        
        # Extract "command" key from dictionary
        command = EmulatorClient.LegalCommands[self.command] + "0" * 16 # Add room for header + data
        # Copy HEADER into command
        command = self.intToByte( HEADER, 0, 3, 8, command)

        try:
            results =''.join(chr(int(data[i:i+2], 16)) for i in range(0, len(data), 2))
            command = command[:20] +  results
        except Exception as e:
            raise EmulatorClientError("Error manipulating '%s' and '%s', because: '%s'" % (results, command, e))
        
        # Transmit command
        try:
            bytesSent = self.sock.send(command)
        except socket.error, e:
            if self.sock:
                self.sock.close()
            raise EmulatorClientError("Error sending %s command: %s" % (command, e))
        else:
            print " %d bytes.." % (bytesSent),
            sys.stdout.flush()

def main():

    try:
        ec = EmulatorClient()
        ec.run()
    except Exception as e:
        print e

if __name__ == '__main__':
    
    main()
