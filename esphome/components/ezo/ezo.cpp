#include "ezo.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ezo {

static const char *const TAG = "ezo.sensor";

void EZOSensor::dump_config() {
  LOG_SENSOR("", "EZO", this);
  LOG_I2C_DEVICE(this);
  if (this->is_failed())
    ESP_LOGE(TAG, "Communication with EZO circuit failed!");
  LOG_UPDATE_INTERVAL(this);
}

void EZOSensor::update() {
  // Check if a read is in there already and if not insert on in the second position

  if (!this->commands_.empty() && this->commands_.front()->command_type != EzoCommandType::EZO_READ &&
      this->commands_.size() > 1) {
    bool found = false;

    for (auto &i : this->commands_) {
      if (i->command_type == EzoCommandType::EZO_READ) {
        found = true;
        break;
      }
    }

    if (!found) {
      EzoCommand *ezo_command = new EzoCommand;
      ezo_command->command = "R";
      ezo_command->command_type = EzoCommandType::EZO_READ;
      ezo_command->delay_ms = 900;

      std::deque<EzoCommand *>::iterator it = this->commands_.begin();
      ++it;
      this->commands_.insert(it, ezo_command);
    }

    return;
  }
<<<<<<< HEAD

  this->get_state();
=======
  if (!this->commands_.empty()) {
    this->add_command("R", "", "");
  }

  ezo_command *to_run = this->commands_.front();

  auto data = reinterpret_cast<const uint8_t *>(&to_run[0]);
  this->write_bytes_raw(data, to_run->command.length());

  this->state_ |= EZO_STATE_WAIT;
  this->start_time_ = millis();
  this->wait_time_ = 900;
>>>>>>> 28a81ece (Initial setup)
}

void EZOSensor::loop() {
  if (this->commands_.empty()) {
    return;
  }

  EzoCommand *to_run = this->commands_.front();

  if (!to_run->command_sent) {
    auto data = reinterpret_cast<const uint8_t *>(&to_run->command.c_str()[0]);
    ESP_LOGVV(TAG, "Sending command \"%s\"", data);

    this->write_bytes_raw(data, to_run->command.length());

    if (to_run->command_type == EzoCommandType::EZO_SLEEP ||
        to_run->command_type == EzoCommandType::EZO_I2C) {  // Commands with no return data
      delete to_run;
      this->commands_.pop_front();
      return;
    }

    this->start_time_ = millis();
    to_run->command_sent = true;
    return;
  }

  if (millis() - this->start_time_ < to_run->delay_ms)
    return;

  uint8_t buf[32];

  buf[0] = 0;

  if (!this->read_bytes_raw(buf, 32)) {
    ESP_LOGE(TAG, "read error");
    delete to_run;
    this->commands_.pop_front();
    return;
  }

  switch (buf[0]) {
    case 1:
      break;
    case 2:
      ESP_LOGE(TAG, "device returned a syntax error");
      break;
    case 254:
      return;  // keep waiting
    case 255:
      ESP_LOGE(TAG, "device returned no data");
      break;
    default:
      ESP_LOGE(TAG, "device returned an unknown response: %d", buf[0]);
      break;
  }

  ESP_LOGVV(TAG, "Received buffer \"%s\" for command type %s", buf, EZO_COMMAND_TYPE_STRINGS[to_run->command_type]);

  // for (int index = 0; index < 32; ++index) {
  //   ESP_LOGD(TAG, "Received buffer index: %d char: \"%c\" %d", index, buf[index], buf[index]);
  // }

  if (buf[0] == 1 || to_run->command_type == EzoCommandType::EZO_CALIBRATION) {  // EZO_CALIBRATION returns 0-3
    std::string payload = reinterpret_cast<char *>(&buf[1]);
    if (!payload.empty()) {
      switch (to_run->command_type) {
        case EzoCommandType::EZO_READ: {
          auto val = parse_float(payload);
          if (!val.has_value()) {
            ESP_LOGW(TAG, "Can't convert '%s' to number!", payload.c_str());
          } else {
            this->publish_state(*val);
          }
          break;
        }
        case EzoCommandType::EZO_LED: {
          this->led_callback_.call(payload.back() == '1');
          break;
        }
        case EzoCommandType::EZO_DEVICE_INFORMATION: {
          int start_location = 0;
          if ((start_location = payload.find(',')) != std::string::npos) {
            this->device_infomation_callback_.call(payload.substr(start_location + 1));
          }
          break;
        }
        case EzoCommandType::EZO_SLOPE: {
          int start_location = 0;
          if ((start_location = payload.find(',')) != std::string::npos) {
            this->slope_callback_.call(payload.substr(start_location + 1));
          }
          break;
        }
        case EzoCommandType::EZO_CALIBRATION: {
          int start_location = 0;
          if ((start_location = payload.find(',')) != std::string::npos) {
            this->calibration_callback_.call(payload.substr(start_location + 1));
          }
          break;
        }
        case EzoCommandType::EZO_T: {
          this->t_callback_.call(payload);
          break;
        }
        case EzoCommandType::EZO_CUSTOM: {
          this->custom_callback_.call(payload);
          break;
        }
        default: {
          break;
        }
      }
    }
  }

<<<<<<< HEAD
  delete to_run;
  this->commands_.pop_front();
=======
  float val = atof((char *) &buf[1]);
  this->publish_state(val);
>>>>>>> 28a81ece (Initial setup)
}

// T
void EZOSensor::set_t(const std::string &value) {
  std::string to_send = "T," + value;
  this->add_command(to_send, EzoCommandType::EZO_T);
}

// Calibration
void EZOSensor::clear_calibration() { this->add_command("Cal,clear", EzoCommandType::EZO_CALIBRATION); }

void EZOSensor::set_calibration(const std::string &point, const std::string &value) {
  std::string to_send = "Cal," + point + "," + value;
  this->add_command(to_send, EzoCommandType::EZO_CALIBRATION, 900);
}

// LED control
void EZOSensor::set_led_state(bool on) {
  std::string to_send = "L,";
  to_send += on ? "1" : "0";
  this->add_command(to_send, EzoCommandType::EZO_LED);
}

// Custom
void EZOSensor::set_custom(const std::string &to_send) { this->add_command(to_send, EzoCommandType::EZO_CUSTOM); }

}  // namespace ezo
}  // namespace esphome
