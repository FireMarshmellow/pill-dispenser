import servo_driver

def web_page():
  with open("Main_page.html", "r", encoding='utf-8') as f:
    html = f.read()
  return html

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(('', 80))
s.listen(5)

#load csv file if it exists, if not generate it with approriate variables

#if exist == true then load vairable data



while True:
  conn, addr = s.accept()
  #print('Got a connection from %s' % str(addr))
  request = conn.recv(1024)
  request = str(request)
  #print('Content = %s' % request)
  
  Contener1button = request.find('/?Contener1')
  if Contener1button == 6:
    print('Contener_1_button_pressed')
    servo_driver.Contener(5) #D1 = 5
    
  Contener2button = request.find('/?Contener2')
  if Contener2button == 6:
    print('Contener_2_button_pressed')
    servo_driver.Contener(4) #D2 = 4 
    
  Contener3button = request.find('/?Contener3')
  if Contener3button == 6:
    print('Contener_3_button_pressed')
    servo_driver.Contener(12) #D6 = 12

  Contener4button = request.find('/?Contener4')
  if Contener4button == 6:
    print('Contener_4_button_pressed')
    servo_driver.Contener(15) #D8 = 15
    
  response = web_page()
  conn.send('HTTP/1.1 200 OK\n')
  conn.send('Content-Type: text/html\n')
  conn.send('Connection: close\n\n')
  conn.sendall(response)
  conn.close()