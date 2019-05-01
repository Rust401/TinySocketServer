project('socket', 'c')

thread_dep = dependency('threads')


executable('server', 'server.c', 'utils.c', dependencies : thread_dep)
executable('client', 'client.c', 'utils.c' ,'mq.c', dependencies : thread_dep)
executable('mqtest', 'mq_test.c', 'mq.c', dependencies : thread_dep)