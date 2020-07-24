import { h, Component } from 'preact';
import style from './style.css';

export default class DeviceTimeSetter extends Component {
  state = {
    error: null,
    browserTime: Date.now(),
    deviceTime: null,
    isLoading: false,
  };

  updateDeviceTime() {
    // TODO make an HTTP request to set the device time with the current browser time.
    console.log("TODO updateDeviceTime");
  }

  componentDidMount() {
    this.timer = setInterval(() => {
      this.setState({ isLoading: true });
      // TODO check the time every minute instead of every second.
      // TODO instead of setInterval we should only schedule another
      //      request after the previous one has finished.
      //      OR abort the existing one before issuing another one.
      //      see https://developer.mozilla.org/en-US/docs/Web/API/AbortController
      fetch("/time.json")
        .then(response => {
          if (response.ok) {
            return response.json();
          } else {
            // TODO include the response in the exception?
            throw new Error('Something went wrong...');
          }
        })
        .then(data => this.setState({
          browserTime: Date.now(),
          deviceTime: data.time*1000,
          isLoading: false,
        }))
        .catch(error => this.setState({
          error,
          browserTime: Date.now(),
          deviceTime: null,
          isLoading: false,
        }));
    }, 1000);
  }

  componentWillUnmount() {
    clearInterval(this.timer);
  }

  render() {
    let deviceTime = new Date(this.state.deviceTime).toISOString();
    let deviceTimeDifferenceSeconds = (this.state.deviceTime - this.state.browserTime) / 1000.0;
    let deviceTimeNeedsAdjustement = Math.abs(deviceTimeDifferenceSeconds) > 60*1000;
    return (
      <div>
        <p className={deviceTimeNeedsAdjustement && style.deviceTimeNeedsAdjustment} title={deviceTimeDifferenceSeconds + "s"}>
          {deviceTime} <button onClick={this.updateDeviceTime}>adjust device time</button>
        </p>
        {this.isLoading && <span>loading</span>}
      </div>
    );
  }
};
