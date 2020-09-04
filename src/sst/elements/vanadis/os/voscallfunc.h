
#ifndef _H_VANADIS_SYSTEM_CALL_FUNC_TYPE
#define _H_VANADIS_SYSTEM_CALL_FUNC_TYPE

namespace SST {
namespace Vanadis {

enum VanadisSyscallOp {
        SYSCALL_OP_UNKNOWN,
        SYSCALL_OP_ACCESS,
        SYSCALL_OP_BRK,
        SYSCALL_OP_SET_THREAD_AREA,
	SYSCALL_OP_UNAME,
	SYSCALL_OP_OPENAT,
	SYSCALL_OP_READ
};

}
}

#endif
