import requests
r = requests.get('http://localhost/?toggle=On%2FOff')
print (r.status_code)
print(r.headers['content-type'])
#s1 = 'http://localhost/?toggle=On%2FOff'
#print (s1)
#s2 = s1.split('?',2)[1]
#print (s2)
