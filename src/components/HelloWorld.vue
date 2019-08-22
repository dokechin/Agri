<template>
  <div class="hello">
    <h1>{{ bleStatus }}</h1>
  </div>
</template>

<script>
export default {
  name: 'HelloWorld',
  props: {
    msg: String
  },
  data: function(){
    return {
      USER_SERVICE_UUID: '作成したLINE TthingsのサービスUUID',
      LED_CHARACTERISTIC_UUID: '独自定義したUUID', // LINE THings Starterのもので大丈夫
      bleConnect: false,
      bleStatus: `デバイスに接続されるのをお待ち下さい。`,
      user: {
        image: '',
        userId: ''
      }
    }
  },
  methods: {
    //BLEが接続できる状態になるまでリトライ
    liffCheckAvailablityAndDo: async function (callbackIfAvailable) {
      try {
        const isAvailable = await this.$liff.bluetooth.getAvailability();
        if (isAvailable) {
          callbackIfAvailable();
        } else {
          // リトライ
          this.bleStatus = `Bluetoothをオンにしてください。`;
            setTimeout(() => this.liffCheckAvailablityAndDo(callbackIfAvailable), 10000);
        }
      } catch (error) {
        alert('Bluetoothをオンにしてください。');
      }
    },
    //サービスとキャラクタリスティックにアクセス
    liffRequestDevice: async function () {
      const device = await this.$liff.bluetooth.requestDevice();
      await device.gatt.connect();
      const service = await device.gatt.getPrimaryService(this.USER_SERVICE_UUID);
      window.ledCharacteristic = await service.getCharacteristic(this.LED_CHARACTERISTIC_UUID);
      // alert('connect');
      this.bleConnect = true;
      this.bleStatus = `デバイスに接続しました。`;
    },
    initializeLiff: async function(){
      await this.$liff.initPlugins(['bluetooth']);
      this.liffCheckAvailablityAndDo(() => this.liffRequestDevice());

      //プロフゲット
      const profile = await this.$liff.getProfile();
      this.user.image = profile.pictureUrl;
      this.user.userId = profile.userId;
    },
  },
  mounted: function(){
    this.$liff.init(
      () => this.initializeLiff()
    );
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
h3 {
  margin: 40px 0 0;
}
ul {
  list-style-type: none;
  padding: 0;
}
li {
  display: inline-block;
  margin: 0 10px;
}
a {
  color: #42b983;
}
</style>
