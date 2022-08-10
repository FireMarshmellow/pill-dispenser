from flask import Flask, render_template

app = Flask('testapp')

@app.route('/')
def index():
    return render_template('config.html', variable='12345')

if __name__ == '__main__':
    app.run(debug=True)