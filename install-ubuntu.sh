sudo rm /usr/bin/python
sudo ln -s /usr/bin/python2 /usr/bin/python

python setup.py v8

sudo rm /usr/bin/python
sudo ln -s /usr/bin/python3 /usr/bin/python

python setup.py spyv8
sudo python setup.py install
