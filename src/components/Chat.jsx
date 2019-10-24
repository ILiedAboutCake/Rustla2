// @flow

import React from 'react';
import cs from 'classnames';

// eslint-disable-next-line no-empty-function
function noop() {}

type Props = {
  className?: string,
  onClose?: () => void,
  src: string,
  style?: Object
};

const Chat = ({ className, onClose = noop, src, style, ...rest }: Props) =>
  <div
    {...rest}
    className={cs('fill-percentage', className)}
    style={{
      ...style,
    }}
    >
    <div>
      <a title='Close' onClick={onClose}>
        <span className='glyphicon glyphicon-remove float-right' />
      </a>
      <a href={src} target='_blank' rel='noopener noreferrer'>
        <span className='glyphicon glyphicon-share-alt float-right' />
      </a>
    </div>
    <iframe
      style={{
        height: 'calc(100% - 1em)',
      }}
      width='100%'
      height='100%'
      marginHeight='0'
      marginWidth='0'
      frameBorder='0'
      scrolling='no'
      src={src}
      />
  </div>
  ;

export default Chat;
