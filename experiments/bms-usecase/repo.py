from os import path

def checkIffileExist(file):
    return path.exists(file)

class Repo(object):
    """
    docstring
    """
    def __init__(self, ):
        pass

    def startRepo(self, node, repoName):
        cmd  = "ndn-python-repo -r /repo/{} >> repo.log 2>&1 &".format(node.name)
        node.cmd(cmd)

    def getDataFromRepo(self, node, repoName, nameAtRepo):
        print("Fetching data from repo")
        cmd = 'getfile.py -r {} -n {} >> repo.log 2>&1 &'.format(repoName, nameAtRepo)
        node.cmd(cmd)

    def putDataIntoRepo(self, node, repoName, nameAtRepo):
        node.cmd('echo "hello-world" > data.file')
        cmd = 'putfile.py -r {} -f data.file -n {} --client_prefix {} >> repo.log 2>&1 &'.format(repoName, nameAtRepo, nameAtRepo)
        node.cmd(cmd)
    
    def deleteDataFromRepo(self):
        pass
