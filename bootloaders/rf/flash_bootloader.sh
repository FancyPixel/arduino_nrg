SCRIPT_PATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
PRODUCT_TYPES_PATH=$SCRIPT_PATH/product_types
LAST_FLASHED_FOR_PATH=${SCRIPT_PATH}/last_flashed_for

declare -a mote_types=("modem" "bollard" "repeater" "gate" "ame")

usage() { echo "Usage: $0 -t <Mote type (modem, bollard, repeater, gate, ame)> [-d <Serial device path>]" 1>&2; exit 1; }
while getopts t:d: option; do
    case "${option}" in
        t) MOTE_TYPE=${OPTARG};;
        d) SERIAL_PORT=${OPTARG};;
        *) usage;;
    esac
done

# Check if MOTE_TYPE has been given
if [ -z "$MOTE_TYPE" ]
then
  echo "Mote type (-t) is mandatory"
  exit 1
fi
# Check if MOTE_TYPE is supported
found=0
for i in "${!mote_types[@]}"
do
  if [ ${mote_types[$i]} == "$MOTE_TYPE" ]; then found=1; fi
done
if [ $found != 1 ]
then
  echo "Mote type '$MOTE_TYPE' not supported"
  exit 1
fi

if [ -z "$SERIAL_PORT" ]
then
  SERIAL_PORT=/dev/tty.usbserial-DA013RBN
fi

# Copy correct product.h based on MOTE_TYPE
cp ${PRODUCT_TYPES_PATH}/${MOTE_TYPE}.h ${SCRIPT_PATH}/product.h

export SERPORT=${SERIAL_PORT}

#last_flashed_for=$(cat $LAST_FLASHED_FOR_PATH)
#if [ "$last_flashed_for" != "$MOTE_TYPE" ]
#then
  make clean
  make
  echo $MOTE_TYPE > $LAST_FLASHED_FOR_PATH
#fi

upload_hex -d $SERIAL_PORT -f rfloader.hex -l info
