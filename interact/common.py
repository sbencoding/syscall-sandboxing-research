import subprocess
import inspect
import signal
import time

def input_case(func):
    func.position = input_case.idx
    input_case.idx += 1
    return func

input_case.idx = 0

class ProgAnalysis():
    def __init__(self, prog_path, opts=None):
        self.prog_path = prog_path
        self.opts = [] if opts is None else opts
        self.syscalls_used = set()
        self.cases_ran = 0
        self.phase_count = 0

    def enable_multi_phase(self, offsets):
        comb_args = zip(['-p' for _ in offsets], [hex(x) for x in offsets])
        self.opts += [x for tup in comb_args for x in tup]
        self.phase_count = len(offsets)
        self.syscalls_used = {}
        for i in range(0, self.phase_count+1):
            self.syscalls_used[i] = set()

    def _process_proc_output(self, proc):
        stdout_data = proc.communicate()[0]
        lines = stdout_data.splitlines()
        if self.phase_count != 0:
            syscalls_per_phase = [ln.decode()[:-1] for ln in lines[-(self.phase_count + 1):]]
            for phase in syscalls_per_phase:
                parts = phase.split(':')
                if len(parts) != 2: continue # phase was not hit, no syscalls to add
                idx, syscalls = parts
                self.syscalls_used[int(idx)] |= set(syscalls.split(','))
        else:
            syscalls = lines[-1].decode()[:-1]
            self.syscalls_used = self.syscalls_used.union(set(syscalls.split(',')))

        return b'\n'.join(lines[:-1])

    def exec(self):
        proc = subprocess.Popen(["./tracer", "-m", *self.opts, '--', self.prog_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return self._process_proc_output(proc)

    def exec_with_args(self, args):
        proc = subprocess.Popen(["./tracer", "-m", *self.opts, '--', self.prog_path, *args], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return self._process_proc_output(proc)
    
    def exec_with_external(self, ext_prog, ext_args):
        proc = subprocess.Popen(["./tracer", "-m", *self.opts, '--', self.prog_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        time.sleep(0.2) # wait for program under test to boot up
        ext_proc = subprocess.Popen([ext_prog, *ext_args])
        ext_proc.wait()
        proc.send_signal(signal.SIGINT)
        return self._process_proc_output(proc)

    def run_all_cases(self):
        print(self.prog_path)
        methods = []
        for k in dir(self):
            method = getattr(self, k)
            if k.startswith('case_') and inspect.ismethod(method) and method.position is not None:
                methods.append((k, method))

        for k,v in sorted(methods, key=lambda el: el[1].position):
            print('-', k)
            v()
            self.cases_ran += 1

        print()

    def stats(self):
        print(self.prog_path, f'; ran {self.cases_ran} times' + (' [MP]' if self.phase_count != 0 else ''), '; used syscalls:', self.syscalls_used)
