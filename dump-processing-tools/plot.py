import dash
import dash_core_components as dcc
import dash_html_components as html
from dash.dependencies import Input, Output
import plotly.express as px
import pandas as pd

df = pd.read_csv('coli2.csv')
dl = pd.read_csv('cilo2.csv')


app = dash.Dash(__name__)

app.layout = html.Div([
    dcc.Graph(id="graph", style={'height': 800}),
    html.Button("Switch Axis", id='btn', n_clicks=0)
])

@app.callback(
    Output("graph", "figure"), 
    [Input("btn", "n_clicks")])
def display_graph(n_clicks):
    fig = px.line(df, x='time', y='brewboiler')    
    fig.add_scatter(x=dl['time'], y=dl['brewboilerhe'], mode='lines', name="Brew boiler HE")
    fig.add_scatter(x=dl['time'], y=dl['serviceboilerhe'], mode='lines', name="Service boiler HE")
    fig.add_scatter(x=df['time'], y=df['serviceboiler'], mode='lines', name="Service boiler")
    fig.add_scatter(x=df['time'], y=df['microswitch'], mode='lines', name="Microswitch")
    fig.add_scatter(x=df['time'], y=df['waterlevel'], mode='lines', name="Water level")
    fig.add_scatter(x=dl['time'], y=dl['pumpon'], mode='lines', name="Pump on")
#    fig.add_scatter(x=dl['time'], y=dl['siga'], mode='lines', name="SigA")
#    fig.add_scatter(x=dl['time'], y=dl['sige'], mode='lines', name="SigE")
#    fig.add_scatter(x=dl['time'], y=dl['sigf'], mode='lines', name="SigF")
#    fig.add_scatter(x=dl['time'], y=dl['sigt'], mode='lines', name="SigT")
    return fig

app.run_server(debug=True)