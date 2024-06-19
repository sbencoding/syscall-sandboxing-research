from common import ProgAnalysis, input_case

class AnlSimplePrint(ProgAnalysis):
    def __init__(self):
        super(AnlSimplePrint, self).__init__('test_progs_out/simple_print')

    @input_case
    def case_default(self):
        self.exec()

class AnlSimplePrintMP(ProgAnalysis):
    def __init__(self):
        super(AnlSimplePrintMP, self).__init__('test_progs_out/simple_print')
        self.enable_multi_phase([0x1147])

    @input_case
    def case_default(self):
        self.exec()

class AnlForking(ProgAnalysis):
    def __init__(self):
        super(AnlForking, self).__init__('test_progs_out/forking')

    @input_case
    def case_default(self):
        self.exec()

class AnlForkingMP(ProgAnalysis):
    def __init__(self):
        super(AnlForkingMP, self).__init__('test_progs_out/forking')
        self.enable_multi_phase([0x118f])

    @input_case
    def case_default(self):
        self.exec()

class AnlMultiForking(ProgAnalysis):
    def __init__(self):
        super(AnlMultiForking, self).__init__('test_progs_out/multi_forking')

    @input_case
    def case_default(self):
        self.exec()

class AnlMultiForkingMP(ProgAnalysis):
    def __init__(self):
        super(AnlMultiForkingMP, self).__init__('test_progs_out/multi_forking')
        self.enable_multi_phase([0x1175, 0x1183])

    @input_case
    def case_default(self):
        self.exec()

class AnlMultiPthread(ProgAnalysis):
    def __init__(self):
        super(AnlMultiPthread, self).__init__('test_progs_out/multi_pthread')

    @input_case
    def case_default(self):
        self.exec()

class AnlMultiPthreadMP(ProgAnalysis):
    def __init__(self):
        super(AnlMultiPthreadMP, self).__init__('test_progs_out/multi_pthread')
        self.enable_multi_phase([0x11db])

    @input_case
    def case_default(self):
        self.exec()

class AnlSimpleExecve(ProgAnalysis):
    def __init__(self):
        super(AnlSimpleExecve, self).__init__('test_progs_out/simple_execve')

    @input_case
    def case_default(self):
        self.exec()

class AnlSimpleExecveMP(ProgAnalysis):
    def __init__(self):
        super(AnlSimpleExecveMP, self).__init__('test_progs_out/simple_execve')
        self.enable_multi_phase([0x11b3])

    @input_case
    def case_default(self):
        self.exec()

if __name__ == '__main__':
    asp = AnlSimplePrint()
    asp.run_all_cases()
    asp.stats()
