import gc

def divide(a,b):
	return a/b

DIX = const(10)

def print_div():
	for i in range(10):
		if i%2 == 0:
			print('le nombre ',i,' est pair et ',i,' divisé par ', DIX, ' = ', divide(i,DIX))
		else:
			print('le nombre ',i,' est impair et ',i,' divisé par ', DIX, ' = ', divide(i,DIX))

def print_tab(a):
	for i in a:
		type_i = str(type(i)) 
		if type_i == "<class 'int'>":
			print(i,' is an int')
		elif type_i == "<class 'str'>":
			print(i,' is a string')
		else:
			print("I don't know the type ", type_i)

tab = [1,2,3,4,5,6,'test']