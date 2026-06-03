# Source this file from the repository root or any shell:
#   source hyperenv.sh

if [ -n "${BASH_SOURCE:-}" ]; then
    _VCU_HYPERENV_PATH="${BASH_SOURCE[0]}"
elif [ -n "${ZSH_VERSION:-}" ]; then
    _VCU_HYPERENV_PATH="${(%):-%x}"
else
    _VCU_HYPERENV_PATH="$0"
fi

export VCU_ROOT="$(cd "$(dirname "$_VCU_HYPERENV_PATH")" && pwd)"
export VCU_STLIB_DIR="$VCU_ROOT/deps/ST-LIB"
export VCU_JSON_ADE_DIR="$VCU_ROOT/Core/Inc/Code_generation/JSON_ADE"

alias cdvcu='cd "$VCU_ROOT"'
alias cdstlib='cd "$VCU_STLIB_DIR"'
alias cdadj='cd "$VCU_JSON_ADE_DIR"'

runeth() {
    cd "$VCU_ROOT"
    ./hyper run main --preset board-debug-eth-ksz8041 --board-name VCU --skip-preflight
    cd -
}

runsingle() {
    cd "$VCU_ROOT"
    ./hyper run main --preset board-debug-eth-lan8700-single --board-name VCU --skip-preflight
    cd -
}

unset _VCU_HYPERENV_PATH
