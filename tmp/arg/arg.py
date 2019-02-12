import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-d", "--debug", action="store_true", 
                    default=False, help="activates debug mode")
parser.add_argument("-c", "--config", type=file, default="server.cfg", 
                    help="adds a configuration file to the server")
parser.add_argument('-u', '--authorised', type=file, default="equips.dat",
                    help='adds a authorised clients file to the server')
args = parser.parse_args()
print args.debug
print args.config.read()
print args.authorised.read()
args.authorised.close()
args.config.close()

