SCRIPT_PATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
PRODUCT_TYPES_PATH=$SCRIPT_PATH/product_types
LAST_MADE_FOR_PATH=${SCRIPT_PATH}/last_made_for

declare -a mote_types=("modem" "bollard" "forklift" "gate" "ame")

usage() { echo "Usage: $0 -t <Mote type (modem, bollard, forklift, gate, ame)> [-s <Serial port path>]" 1>&2; exit 1; }
while getopts t:s: option; do
    case "${option}" in
        t) MOTE_TYPE=${OPTARG};;
        s) SERIAL_PORT=${OPTARG};;
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

last_made_for=$(cat $LAST_MADE_FOR_PATH)
if [ "$last_made_for" != "$MOTE_TYPE" ]
then
  make clean
  make
  echo $MOTE_TYPE > $LAST_MADE_FOR_PATH
fi

make bsl-flash
