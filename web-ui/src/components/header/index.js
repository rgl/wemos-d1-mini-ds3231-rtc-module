import { Link } from 'preact-router/match';
import style from './style.css';

export default () => (
  <header class={style.header}>
    <h1><a href="/">wemos rtc</a></h1>
    <nav>
      <Link activeClassName={style.active} href="/">Home</Link>
    </nav>
  </header>
);
