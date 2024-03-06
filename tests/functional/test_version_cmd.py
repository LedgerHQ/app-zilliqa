from apps.zilliqa import ZilliqaClient

# Taken from the Makefile, to update every time the Makefile version is bumped
MAJOR = 0
MINOR = 5
PATCH = 3


# In this test we check the behavior of the device when asked to provide the app version
def test_version(backend):
    # Use the app interface instead of raw interface
    client = ZilliqaClient(backend)
    # Send the GET_VERSION instruction
    version = client.send_get_version()
    assert version == (MAJOR, MINOR, PATCH)
