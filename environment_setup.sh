#!/bin/bash
#
#  Copyright 2024-2025 NXP
#
#  SPDX-License-Identifier: BSD-3-Clause
#

app_submodules()
{
    echo 'Setting up submodules.'
    git submodule update --init --remote
    git submodule update --init --recursive
    
    echo 'Setting up submodules done.'
}

sdk_pull()
{
    echo 'Pulling NXP SDK.'
    cd ot-nxp/third_party/github_sdk/
    west init -l manifest --mf west.yml
    west update

    cd ../../../
}

patch_ot_repo()
{
    # apply ot-nxp repo patch
    cd ot-nxp/
    git apply ../patches/ot-nxp-freertos.patch
    cd ..

    echo "ot-nxp repo has been patched!"
}

begin_setup_repos()
{
    echo 'The script will update all repositories (ot-nxp and NXP SDK).'
    echo 'The setup will take several minutes to complete.'

    app_submodules

    sdk_pull

    echo 'Patching ot-nxp repo.'
    patch_ot_repo
}

begin_setup_tools()
{
    echo 'The script will update the toolchain.'
    echo 'The setup will take several minutes to complete.'

    cd ot-nxp
    echo 'Setting up ot-nxp toolchain environment.'
    ./script/bootstrap

    echo 'Installing west.'
    pip install west

    cd ..

    echo 'Toolchains setup done.'
}

clean_app_repo()
{
    cd ot-nxp
    echo 'Resetting ot-nxp repository.'
    git reset --hard
    cd third_party/github_sdk
    echo 'Cleaning NXP SDK.'
    west forall -c "git reset --hard && git clean -xdf" -a
    cd ../../..
}

echo 'This is a setup script to set the NXP port of KNX IoT OpenThread examples environment.'

case $1 in
  clean)
    echo "Cleaning the environment."
    clean_app_repo
    ;;
  setup_repos)
    echo "Setting up the repositories."
    begin_setup_repos
    ;;
  setup_tools)
    echo "Setting up the toolchain."
    begin_setup_tools
    ;;
  apply_ot_patch)
    echo "Applying ot-nxp repo patch."
    patch_ot_repo
    ;;
  *)
    echo "Usage: $0 clean|setup_repos|setup_tools|apply_ot_patch"
    ;;
esac