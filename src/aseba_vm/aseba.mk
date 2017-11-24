ASEBA = ./aseba

ASEBASRC = $(ASEBA)/transport/buffer/vm-buffer.c \
           $(ASEBA)/transport/can/can-buffer.c \
           $(ASEBA)/transport/can/can-net.c \
           $(ASEBA)/vm/natives.c \
           $(ASEBA)/vm/vm.c \
           src/aseba_vm/aseba_can_interface.c \
           src/aseba_vm/aseba_node.c \
           src/aseba_vm/skel_user.c \
           src/aseba_vm/aseba_bridge.c \


ASEBAINC = $(ASEBA)
