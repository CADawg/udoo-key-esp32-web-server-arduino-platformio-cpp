from os.path import isfile
# noinspection PyUnresolvedReferences
Import("env")
assert isfile(".env")
try:
    f = open(".env", "r")
    lines = f.readlines()
    envs = []
    for line in lines:
        envs.append("-D{}".format(line.strip()))
    # noinspection PyUnresolvedReferences
    env.Append(BUILD_FLAGS=envs)
except IOError:
    print("File .env not accessible",)
finally:
    f.close()