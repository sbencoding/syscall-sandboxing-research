from common import ProgAnalysis
import anl_simple_progs

all_progs = ProgAnalysis.__subclasses__()
instances = []

for prog in all_progs:
    inst = prog()
    instances.append(inst)
    inst.run_all_cases()

for inst in instances:
    inst.stats()
