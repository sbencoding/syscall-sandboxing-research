from common import ProgAnalysis, input_case

class AnlRedis(ProgAnalysis):
    def __init__(self):
        super(AnlRedis, self).__init__('/usr/bin/redis-server')

    @input_case
    def case_bigtest(self):
        self.exec_with_external('/usr/bin/redis-benchmark', ['-n', '10'])

class AnlRedisMP(AnlRedis):
    def __init__(self):
        super(AnlRedisMP, self).__init__()
        # `aeMain` function
        self.enable_multi_phase([0x00000000000274e0])

if __name__ == '__main__':
    asp = AnlRedisMP()
    asp.run_all_cases()
    asp.stats()
