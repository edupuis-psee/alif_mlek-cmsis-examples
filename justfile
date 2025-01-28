solution_name := 'mlek'
solution := solution_name + '.csolution.yml'
python := `which python`
cbuild_idx := solution_name + '.cbuild-idx.yml'

# Build a context. A context must be {{project}}.{{build_type}}+{{target}} (ex: blinky.debug+HE, qsort.release+HP).
build context flags='':
    cbuild {{solution}} --packs -c {{context}} -S {{flags}}

# Prepare the security with a context. A context must be {{project}}.{{build_type}}+{{target}} (ex: blinky.debug+HE, qsort.release+HP).
prepare context flags='': (build context flags)
    #!/bin/bash
    set -euox pipefail
    read bin target < <({{python}} scripts/get_binary_from_target_from_last_build.py {{cbuild_idx}})
    if [ "$bin" != "" ] && [ "$target" != "" ]; then
        cp $bin /home/edupuis/app-release-exec-linux/build/images/alif-img.bin
        cp .alif/M55_${target}_cfg.json /home/edupuis/app-release-exec-linux/alif-img.json
    else
        echo "Error getting the build bin ($bin) or target ($target), at least one is empty"
        exit 1
    fi

# Flash a context. A context must be {{project}}.{{build_type}}+{{target}} (ex: blinky.debug+HE, qsort.release+HP).
flash context flags='': (prepare context flags)
    #!/bin/bash
    set -euox pipefail
    pushd /home/edupuis/app-release-exec-linux
    ./app-gen-toc -f alif-img.json
    ./app-write-mram -p
    rm ./build/images/alif-img.bin
    rm ./alif-img.json
    popd

prepare_flash_custom bin:
    #!/bin/bash
    set -euox pipefail
    cp {{ bin }} /home/edupuis/app-release-exec-linux/build/images/alif-img.bin
    cp /work/dev/alif_ml-embedded-evaluation-kit/alif_hp.json /home/edupuis/app-release-exec-linux/alif-img.json
    pushd /home/edupuis/app-release-exec-linux
    ./app-gen-toc -f alif-img.json
    ./app-write-mram -p
    rm ./build/images/alif-img.bin
    rm ./alif-img.json
    popd

flash_ext serial_port bin_to_flash:
    #!/bin/bash
    set -euox pipefail
    echo "Flashing device"
    pushd /home/edupuis/app-release-exec-linux
    ./app-gen-toc -f build/config/alif_usb-to-ospi-flasher.json
    ./app-write-mram -p
    popd
    echo "Wait for device to be ready (15s)"
    sleep 15
    echo "Sending file over xmodem"
    stty -F {{ serial_port }} 115200 cs8 -parenb -cstopb -ixoff
    stty -F {{ serial_port }}
    sx -vv {{ bin_to_flash }} < {{ serial_port }} > {{ serial_port }}
