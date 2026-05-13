# Memory operations and R0 protection.
MOVI R1 1024
MOVI R2 77
MOVM R2 R1 0
MOVR R3 R1 0
MOVI R0 999
ADD R0 R3 R4
