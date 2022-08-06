import servo_driver

def web_page():
  with open("Main_page.html", "r", encoding='utf-8') as f:
    html = f.read()
  return html

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(('', 80))
s.listen(5)

while True:
  conn, addr = s.accept()
  print('Got a connection from %s' % str(addr))
  request = conn.recv(1024)
  request = str(request)
  print('Content = %s' % request)
  
  Contener1button = request.find('/?Contener1')
  if Contener1button == 6:
    print('Contener_1_button_pressed')
    servo_driver.Contaner1()
    
  Contener2button = request.find('/?Contener2')
  if Contener2button == 6:
    print('Contener_2_button_pressed')
    servo_driver.Contaner2()
    
  Contener3button = request.find('/?Contener3')
  if Contener3button == 6:
    print('Contener_3_button_pressed')
    servo_driver.Contaner3()

  Contener4button = request.find('/?Contener4')
  if Contener4button == 6:
    print('Contener_4_button_pressed')
    servo_driver.Contaner4()
    
  response = web_page()
  conn.send('HTTP/1.1 200 OK\n')
  conn.send('Content-Type: text/html\n')
  conn.send('Connection: close\n\n')
  conn.sendall(response)
  conn.close()
