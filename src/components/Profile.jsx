import React from 'react';
import PropTypes from 'prop-types';
import { compose } from 'redux';
import { connect } from 'react-redux';
import cs from 'classnames';
import idx from 'idx';
import lifecycle from 'recompose/lifecycle';
import trim from 'lodash/trim';
import qs from 'qs';

import { fetchProfile, updateProfile } from '../actions';
import Checkbox from './Checkbox';
import MainLayout from './MainLayout';
import ServiceSelect from './ServiceSelect';


const Profile = ({
  history,
  profile,
  updateProfile,
  isUsernameSet,
  defaultUsername,
}) =>
  <MainLayout history={history}>
    <div className='container'>
      <h1 className='mt-4 text-center'>Settings</h1>
      {profile.isFetching ? <div className='h3 text-center'>LOADING...</div> : null}
      {profile.data ?
        <form
          className='form-horizontal'
          onSubmit={event => {
            event.preventDefault();
            const payload = {
              ...profile.data,
              service: event.target.elements.service.value,
              channel: event.target.elements.channel.value,
              stream_path: event.target.elements.stream_path.value,
              username: event.target.elements.username.value,
              left_chat: event.target.elements.left_chat.checked,
              show_hidden: event.target.elements.show_hidden.checked,
              show_dgg_chat: event.target.elements.show_dgg_chat.checked,
            };
            // Disallow blank inputs. The ORM will disallow these anyways but
            // there's arguably no point in even making the request if it's known
            // ahead of time that it's gonna fail. This check might be better off
            // in the `updateProfile` action itself. Also, would be nice if we
            // visually showed the user an error message regarding this (or simply
            // disabled the button).
            if (!payload.channel || !payload.channel.length) {
              return;
            }

            updateProfile(payload);
          }}
          >
          {!isUsernameSet ?
            <div className='form-group'>
              <div className='col-sm-2' />
              <div className='col-sm-10'>
                <strong className='profile-first-login-message'>Save your profile to finish logging in.</strong>
              </div>
            </div>
          : null}
          <div className={cs('form-group', { 'form-group-highlight': !isUsernameSet })}>
            <label htmlFor='profile-username'>Username</label>
            <input
              className='form-control'
              id='profile-username'
              type='text'
              name='username'
              defaultValue={profile.data.username || defaultUsername}
              />
            <small className='form-text text-muted'>Choose carefully, your username cannot be changed!</small>
          </div>
          <div className='form-group'>
            <label htmlFor='profile-channel'>Stream Path</label>
            <input
              className='form-control'
              id='profile-stream-path'
              type='text'
              name='stream_path'
              defaultValue={profile.data.stream_path}
              />
            <small className='form-text text-muted'>{location.origin}/<strong>my_stream_path</strong></small>
          </div>
          <div className="form-row">
            <div className="form-group col-md-6">
              <label htmlFor='profile-service-select'>Streaming Service</label>
              <ServiceSelect id='profile-service-select' defaultValue={profile.data.service} />
            </div>
            <div className="form-group col-md-6">
              <label htmlFor='profile-channel'>Channel/Video ID</label>
              <input
                className='form-control'
                id='profile-channel'
                type='text'
                name='channel'
                defaultValue={profile.data.channel}
              />
            </div>
          </div>
          <div className='form-group'>
            <div className='form-check'>
              <Checkbox
                id='profile-leftchat'
                name='left_chat'
                defaultChecked={profile.data.left_chat}
              />
              <label htmlFor='profile-leftchat' className='form-check-label'>Use Left Chat</label>
            </div>
          </div>
          <div className='form-group'>
            <div className='form-check'>
              <Checkbox
                id='profile-showhidden'
                name='show_hidden'
                defaultChecked={profile.data.show_hidden}
              />
              <label htmlFor='profile-showhidden' className='form-check-label'>Show Hidden Streams</label>
            </div>
          </div>
          <div className='form-group'>
            <div className='form-check'>
              <Checkbox
                id='profile-showdgg'
                name='show_dgg_chat'
                defaultChecked={profile.data.show_dgg_chat}
              />
              <label htmlFor='profile-showdgg' className='form-check-label'>Show DGG Chat With AngelThump</label>
            </div>
          </div>
          <button type='submit' className='btn btn-primary' disabled={profile.isFetching}>Save Changes</button>
        </form>
      : null}
      {profile.err ?
          <div className='alert alert-danger' role='alert'>
            Error: {profile.err.message || 'Failed to fetch profile'}
          </div>
      : null}
    </div>
  </MainLayout>
  ;

Profile.propTypes = {
  history: PropTypes.object.isRequired,
  profile: PropTypes.shape({
    data: PropTypes.shape({
      username: PropTypes.string,
      service: PropTypes.string,
      channel: PropTypes.string,
    }),
    err: PropTypes.any,
    isFetching: PropTypes.bool,
    isUsernameSet: PropTypes.bool,
    defaultUsername: PropTypes.string,
  }),
  updateProfile: PropTypes.func.isRequired,
  isUsernameSet: PropTypes.bool,
  defaultUsername: PropTypes.string.isRequired
};

function mapDispatchToProps(dispatch) {
  return {
    fetchProfile(history) {
      return dispatch(fetchProfile(history));
    },
    updateProfile(data) {
      return dispatch(updateProfile(data));
    },
  };
}

export default compose(
  connect(
    (state, ownProps) => ({
      profile: state.self.profile,
      isUsernameSet: Boolean(idx(state, _ => _.self.profile.data.username)),
      defaultUsername: qs.parse(trim(ownProps.location.search, '?')).username || '',
    }),
    mapDispatchToProps,
  ),
  lifecycle({
    componentDidMount() {
      this.props.fetchProfile(this.props.history);
    },
  }),
)(Profile);
