import dash
import dash_core_components as dcc
import dash_html_components as html
from dash.dependencies import Input, Output
import plotly.express as px
import pandas as pd

df = pd.read_csv('processed2.csv')

app = dash.Dash(__name__)

app.layout = html.Div([
    dcc.Graph(id="graph", style={'height': 800}),
    html.Button("Switch Axis", id='btn', n_clicks=0)
])

@app.callback(
    Output("graph", "figure"), 
    [Input("btn", "n_clicks")])
def display_graph(n_clicks):
    fig = px.line(df, x='time', y='b8')    
#    fig.add_scatter(x=df['time'], y=df['b5'], mode='lines')
#    fig.add_scatter(x=df['time'], y=df['b8'], mode='lines')
    fig.add_scatter(x=df['time'], y=df['b11'], mode='lines')
    fig.add_scatter(x=df['time'], y=df['b14'], mode='lines')
#    fig.add_scatter(x=df['time'], y=df['b16'], mode='lines')

    return fig

app.run_server(debug=True)