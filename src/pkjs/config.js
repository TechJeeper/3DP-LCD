module.exports = [
  {
    type: 'heading',
    defaultValue: '3DP LCD'
  },
  {
    type: 'text',
    defaultValue: 'Nozzle = hour, Bed = minute, progress = how much of the day has passed.'
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Time'
      },
      {
        type: 'select',
        messageKey: 'HourMode',
        defaultValue: '0',
        label: 'Hour format',
        options: [
          { label: 'System setting', value: '0' },
          { label: '12-hour', value: '1' },
          { label: '24-hour', value: '2' }
        ]
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Display'
      },
      {
        type: 'toggle',
        messageKey: 'ShowProgress',
        label: 'Day progress bar',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'ShowXYZ',
        label: 'XYZ line (month / day / seconds)',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'ShowRemaining',
        label: 'Remaining time + status',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'ShowWeekday',
        label: 'Weekday .GCO filename',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'FanSpin',
        label: 'Spinning fan',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'ShowFeedrate',
        label: 'Show F100%',
        defaultValue: true
      }
    ]
  },
  {
    type: 'section',
    capabilities: ['COLOR'],
    items: [
      {
        type: 'heading',
        defaultValue: 'Colors'
      },
      {
        type: 'color',
        messageKey: 'BackgroundColor',
        defaultValue: '000055',
        label: 'Background',
        sunlight: false
      },
      {
        type: 'color',
        messageKey: 'PanelColor',
        defaultValue: '0000AA',
        label: 'LCD panel',
        sunlight: false
      },
      {
        type: 'color',
        messageKey: 'TextColor',
        defaultValue: 'FFFFFF',
        label: 'Primary text',
        sunlight: false
      },
      {
        type: 'color',
        messageKey: 'AccentColor',
        defaultValue: '55AAFF',
        label: 'Labels / accents',
        sunlight: false
      },
      {
        type: 'color',
        messageKey: 'BarColor',
        defaultValue: 'FFFFFF',
        label: 'Progress bar fill',
        sunlight: false
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
